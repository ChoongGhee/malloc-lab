/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t blocksize);
static void place(void *bp, size_t blocksize);

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 초기화 매크로 함수
#define WSIZE 4 // header/footer 크기 = 1 word
#define DSIZE 8
#define CHUNKSIZE (1 << 12) // 힙 메모리를 늘릴때의 기준 2^12의 값(4096, 8의 512배)

#define MAX(x, y) ((x) > (y) ? (x) : (y)) // 크기 비교

// 헤더/풋터에 저장할 값을 리턴 (size는 000빼고 나머지 그리고 alloc은 000부분)
#define PACK(size, alloc) ((size) | (alloc))

// void형인 p의 주소에 역참조(dereferencing) 함.
#define GET(p) (*(unsigned int *)(p))
// void형인 p의 주소에 역참조(dereferencing) 하여 해당 데이터를 val로 바꿈.
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// 헤더/풋터에 저장된 내용을 통해 정보를 얻음.
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)

// malloc의 할당 포인터 기준 -1 wsize면 헤더, 헤더를 읽어서 크기만큼 더하고 -2wsize(헤더 + 풋터)를 하면 풋터 주소
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 묵시적(헤더/풋터)일 경우 다음과 이전 bp 주소를 받게끔 하는 매크로
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define GET_NEXT(bp) GET(HDRP(NEXT_BLKP(bp)))
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
// 8의 배수를 맞추어줌. 자연스레 패딩을 해줌.
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
// first_fit 정책을 위한 기준점 heap_listp
static void *heap_listp;
static char *find_brk;
int mm_init(void)
{

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 0x3));
    // 할당기의 기준을 8의 배수에 맞춰줌.
    heap_listp += (2 * WSIZE);
    find_brk = (char *)heap_listp + DSIZE;
    // extend_heap(1);
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}
static void *extend_heap(size_t words)
{

    char *bp;
    size_t size;

    // 만약 홀수라면 8의 배수를 맞추기 위해 1을 더함. 왜냐 4를 곱하기 때문에 항상 2의 배우여야 주소가 8의 배수가 됨.
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    size_t a = 0;
    if (GET_PREV_ALLOC(HDRP(bp)) == 2)
    {
        a = 2;
    }
    // 헤더에 size와 할당값을 넣어줌 > 즉, size(청크)만큼의 가용 블록이 있다고 알려줌.
    PUT(HDRP(bp), PACK(size, a));
    // 풋터에도 마찬가지 > 1.헤더의 값을 읽고 나서 2.그만큼 이동 후 3. PACK을 해줌.
    PUT(FTRP(bp), PACK(size, a));
    // (에필로그) 삽입
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // bp기준으로 경계 태그를 이용해 연결
    return coalesce(bp);
}
static void *coalesce(void *bp)
{

    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        PUT(HDRP(bp), PACK(size, 0x2));
        find_brk = bp;
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0x2));
        PUT(FTRP(bp), PACK(size, 0x2));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0x2));
        PUT(FTRP(bp), PACK(size, 0x2));
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0x2));
        PUT(FTRP(bp), PACK(size, 0x2));
    }

    find_brk = bp;
    return bp;
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t blocksize;
    size_t extendedsize;
    char *bp;
    if (size == 0)
        return NULL;

    // 헤더와 풋터의 데이터을 더하고 8의 배수(패딩)시켜줌.
    blocksize = ALIGN(size + WSIZE);

    if ((bp = find_fit(blocksize)) != NULL)
    {
        place(bp, blocksize);
        return bp;
    }

    extendedsize = MAX(blocksize, CHUNKSIZE);
    if ((bp = extend_heap(extendedsize / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, blocksize);
    return bp;
}
static void *find_fit(size_t blocksize)
{
    char *temp = find_brk;

    while (GET_SIZE(HDRP(temp)) != 0)
    {
        if (GET_ALLOC(HDRP(temp)) == 0 && blocksize <= GET_SIZE(HDRP(temp)))
        {
            find_brk = temp;
            return temp;
        }
        temp = NEXT_BLKP(temp);
    }

    // If not found, search from the beginning of the heap to find_brk
    temp = heap_listp + DSIZE;
    while (temp < (char *)find_brk)
    {
        if (GET_ALLOC(HDRP(temp)) == 0 && blocksize <= GET_SIZE(HDRP(temp)))
        {
            find_brk = temp;
            return temp;
        }
        temp = NEXT_BLKP(temp);
    }

    // If still not found, return NULL
    return NULL;
}

static void place(void *bp, size_t blocksize)
{
    size_t a = 0;
    if (GET_PREV_ALLOC(HDRP(bp)) == 2)
    {
        a = 2;
    }

    size_t old_size = GET_SIZE(HDRP(bp));
    if (old_size - blocksize >= (DSIZE))
    {
        PUT(HDRP(bp), PACK(blocksize, 1 + a));
        // PUT(FTRP(bp), PACK(blocksize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(old_size - blocksize, 0x2));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(old_size - blocksize, 0x2));
    }
    else
    {
        PUT(HDRP(bp), PACK(old_size, 1 + a));
        PUT(HDRP(NEXT_BLKP(bp)), (GET_NEXT(bp) | 0x2));
        // PUT(FTRP(bp), PACK(old_size, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t a = 0;
    if (GET_PREV_ALLOC(HDRP(ptr)) == 2)
    {
        a = 2;
    }
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, a));
    PUT(FTRP(ptr), PACK(size, a));

    PUT(HDRP(NEXT_BLKP(ptr)), (GET_NEXT(ptr) - 0x2));
    // PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL)
    {
        return NULL;
    }
    size_t csize = GET_SIZE(HDRP(ptr));
    if (size < csize)
    {                 // 재할당 요청에 들어온 크기보다, 기존 블록의 크기가 크다면
        csize = size; // 기존 블록의 크기를 요청에 들어온 크기 만큼으로 줄인다.
    }
    memcpy(new_ptr, ptr, csize); // ptr 위치에서 csize만큼의 크기를 new_ptr의 위치에 복사함
    mm_free(ptr);                // 기존 ptr의 메모리는 할당 해제해줌
    return new_ptr;
}
