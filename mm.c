#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
// #include "memlib.c"

team_t team = {
    "ateam",            // Team name
    "Harry Bovik",      // First member's full name
    "bovik@cs.cmu.edu", // First member's email address
    "",                 // Second member's full name (leave blank if none)
    ""                  // Second member's email address (leave blank if none)
};

typedef struct NODE
{
    struct NODE *prev;
    struct NODE *next;
} node;

static void *extend_heap(size_t words);
static void *coalesce(node *bp);
static void *find_fit(size_t blocksize);
static void place(void *bp, size_t blocksize);
static void insert_available_block(node *bp);
static void remove_block_from_list(node *bp);

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define HEAD_SIZE 4

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define GET_NEXT(bp) (GET(HDRP(NEXT_BLKP(bp))))

#define ALIGNMENT 8

#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define ALIGNED_SIZE(size) ((size + HEAD_SIZE + DSIZE - 1) & ~(DSIZE - 1))

static void *heap_listp;
// void *a_infive;
// size_t bbbb;

// int main()
// {
//     node *test, *test2, *test3, *test4, *test5, *test6;

//     mem_init();
//     mm_init();

//     test = (node *)mm_malloc(2040);
//     test2 = (node *)mm_malloc(4010);
//     test3 = (node *)mm_malloc(48);
//     test4 = (node *)mm_malloc(4072);
//     test5 = (node *)mm_malloc(4072);
//     test6 = (node *)mm_malloc(4072);

//     mm_free(test);
//     mm_free(test2);
//     mm_free(test3);
//     mm_free(test4);
//     mm_free(test5);
//     mm_free(test6);

//     return 0;
// }

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(24)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);                                // Alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(2 * DSIZE, 1)); // Prologue header
    PUT(heap_listp + (2 * WSIZE), (unsigned int)NULL); // Prologue PRED pointer
    PUT(heap_listp + (3 * WSIZE), (unsigned int)NULL); // Prologue SUCC pointer
    PUT(heap_listp + (4 * WSIZE), PACK(2 * DSIZE, 1)); // Prologue footer
    PUT(heap_listp + (5 * WSIZE), PACK(0, 0x3));       // Epilogue header

    heap_listp = (char *)heap_listp + (2 * WSIZE);
    ((node *)heap_listp)->prev = NULL;
    ((node *)heap_listp)->next = NULL;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    size_t prev_alloc = 0;
    if (GET_PREV_ALLOC(HDRP(bp)) == 2)
    {
        prev_alloc = 2;
    }

    PUT(HDRP(bp), PACK(size, prev_alloc)); // Set header
    PUT(FTRP(bp), PACK(size, prev_alloc)); // Set footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // Set new epilogue header

    insert_available_block((node *)bp);

    return coalesce((node *)bp);
}

static void *coalesce(node *bp)
{
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        PUT(HDRP(bp), PACK(size, 0x2));
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        remove_block_from_list((node *)NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    }
    else if (!prev_alloc && next_alloc)
    {
        remove_block_from_list((node *)PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = (node *)PREV_BLKP(bp);
    }
    else
    {
        remove_block_from_list((node *)PREV_BLKP(bp));
        remove_block_from_list((node *)NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        bp = (node *)PREV_BLKP(bp);
    }

    PUT(HDRP(bp), PACK(size, 0x2));
    PUT(FTRP(bp), PACK(size, 0x2));

    insert_available_block(bp);
    return bp;
}

static void insert_available_block(node *bp)
{
    node *temp = ((node *)heap_listp)->next;

    bp->next = temp;
    bp->prev = (node *)heap_listp;

    if (temp != NULL)
    {
        temp->prev = bp;
    }

    ((node *)heap_listp)->next = bp;
}

static void remove_block_from_list(node *bp)
{
    if (bp->prev != NULL)
        bp->prev->next = bp->next;
    if (bp->next != NULL)
        bp->next->prev = bp->prev;
}

void *mm_malloc(size_t size)
{
    size_t blocksize;
    size_t extendedsize;
    char *bp;

    if (size == 0)
        return NULL;

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
    node *temp = ((node *)heap_listp)->next;

    while (temp != NULL)
    {

        if (blocksize <= GET_SIZE(HDRP(temp)))
        {
            return temp;
        }

        temp = temp->next;
    }
    return NULL;
}

static void place(void *bp, size_t blocksize)
{
    size_t prev_alloc = 0;
    if (GET_PREV_ALLOC(HDRP(bp)) == 2)
    {
        prev_alloc = 2;
    }
    remove_block_from_list((node *)bp);

    size_t old_size = GET_SIZE(HDRP(bp));
    if (old_size - blocksize >= DSIZE)
    {
        PUT(HDRP(bp), PACK(blocksize, 1 + prev_alloc));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(old_size - blocksize, 0x2));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(old_size - blocksize, 0x2));

        insert_available_block((node *)NEXT_BLKP(bp));
    }
    else
    {
        PUT(HDRP(bp), PACK(old_size, 1 + prev_alloc));
        PUT(HDRP(NEXT_BLKP(bp)), GET(HDRP(NEXT_BLKP(bp))) | 0x2);
    }
}

void mm_free(void *ptr)
{
    size_t prev_alloc = 0;
    if (GET_PREV_ALLOC(HDRP(ptr)) == 2)
    {
        prev_alloc = 2;
    }
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, prev_alloc));
    PUT(FTRP(ptr), PACK(size, prev_alloc));

    if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
    {
        PUT(HDRP(NEXT_BLKP(ptr)), GET(HDRP(NEXT_BLKP(ptr))) & ~0x2);
    }

    remove_block_from_list(ptr);
    coalesce((node *)ptr);
}

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

    size_t old_size = GET_SIZE(HDRP(ptr));
    if (size < old_size)
    {
        old_size = size;
    }

    memcpy(new_ptr, ptr, old_size);
    mm_free(ptr);

    return new_ptr;
}