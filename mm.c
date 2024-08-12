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

// malloc의 할당 포인터 기준 -1 wsize면 헤더, 헤더를 읽어서 크기만큼 더하고 -2wsize(헤더 + 풋터)를 하면 풋터 주소
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 묵시적(헤더/풋터)일 경우 다음과 이전 bp 주소를 받게끔 하는 매크로
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
// 8의 배수를 맞추어줌. 자연스레 패딩을 해줌.
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
// first_fit 정책을 위한 기준점 heap_listp
static void *heap_listp;

int mm_init(void)
{

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    // 할당기의 기준을 8의 배수에 맞춰줌.
    heap_listp += (2 * WSIZE);

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

    // 헤더에 size와 할당값을 넣어줌 > 즉, size(청크)만큼의 가용 블록이 있다고 알려줌.
    PUT(HDRP(bp), PACK(size, 0));
    // 풋터에도 마찬가지 > 1.헤더의 값을 읽고 나서 2.그만큼 이동 후 3. PACK을 해줌.
    PUT(FTRP(bp), PACK(size, 0));
    // 다음 헤더를 size가 0이고 할당한 것으로 해줌. (에필로그)
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // bp기준으로 경계 태그를 이용해 연결
    return coalesce(bp);
}
static void *coalesce(void *bp)
{
    // case 2,3,4 코드와 다름. 내생각엔 노상관인듯.
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

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
    blocksize = ALIGN(size + SIZE_T_SIZE);

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
    char *temp = heap_listp + DSIZE;

    while (GET_SIZE(HDRP(temp)) != 0)
    {
        if (GET_ALLOC(HDRP(temp)) == 0 && blocksize <= GET_SIZE(HDRP(temp)))
        {
            return temp;
        }
        temp = NEXT_BLKP(temp);
    }
    return NULL;
    // char *temp;
    // for (temp = heap_listp; GET_SIZE(HDRP(temp)) > 0; temp = NEXT_BLKP(temp))
    // {
    //     if (!GET_ALLOC(HDRP(temp)) && (blocksize <= GET_SIZE(HDRP(temp))))
    //     {
    //         return temp;
    //     }
    // }
    // return NULL;
}

static void place(void *bp, size_t blocksize)
{
    size_t old_size = GET_SIZE(HDRP(bp));
    if (old_size - blocksize >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(blocksize, 1));
        PUT(FTRP(bp), PACK(blocksize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(old_size - blocksize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(old_size - blocksize, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(old_size, 1));
        PUT(FTRP(bp), PACK(old_size, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - WSIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
