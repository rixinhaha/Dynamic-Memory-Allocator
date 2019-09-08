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
  * provide your information in the following struct.
  ********************************************************/
team_t team = {
	/* Your UID */
	"UID305088388",
	/* Your full name */
	"RI XIN WANG",
	/* Your email address */
	"rixin@ymail.com",
	/* Leave blank */
	"",
	/* Leave blank */
	""
};

/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

/* The only global variable is a pointer to the first block */
static char *heap_listp;



/* single word (4) or double word (8) alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/////////////PART 2 MACROS for freelist/////////////////////////////
/* global variable for free list*/
#define ELEMENTS 20

/* global variable for free list*/
void *free_list[ELEMENTS]; //doubly linked list

int getindex(size_t size)
{
	if (size <= (1 << 1))
		return 0;
	else if (size <= (1 << 2))
		return 1;
	else if (size <= (1 << 3))
		return 2;
	else if (size <= (1 << 4))
		return 3;
	else if (size <= (1 << 5))
		return 4;
	else if (size <= (1 << 6))
		return 5;
	else if (size <= (1 << 7))
		return 6;
	else if (size <= (1 << 8))
		return 7;
	else if (size <= (1 << 9))
		return 8;
	else if (size <= (1 << 10))
		return 9;
	else if (size <= (1 << 11))
		return 10;
	else if (size <= (1 << 12))
		return 11;
	else if (size <= (1 << 13))
		return 12;
	else if (size <= (1 << 14))
		return 13;
	else if (size <= (1 << 15))
		return 14;
	else if (size <= (1 << 16))
		return 15;
	else if (size <= (1 << 17))
		return 16;
	else if (size <= (1 << 18))
		return 17;
	else if (size <= (1 << 19))
		return 18;
	else
		return 19;
}

/* Set pointer value */
#define PUTPTR(p, val) (*(unsigned long*)(p) = (val))
#define GETPTR(ptr)	(*(unsigned long *)(ptr))

/* The address of the previous pointer and the next pointer stored in the unallocated block */
#define NEXT_PTR(ptr) ((char*)(ptr))
#define PREV_PTR(ptr) ((char*)(ptr + DSIZE))

//////////////////////////////////////////
//heap checker function
//////////////////////////////////////////

//heap checker function

void mm_check(void)
{
	//check every block in the free list market as free
	int i;
	for (i = 0; i < ELEMENTS; i++)
	{
		void* bp = free_list[i];
		while (bp != NULL)
		{
			if (GET_ALLOC(HDRP(bp)) != 0)
			{
				printf("The block at address %x is not marked as free!\n", bp);
				break;
			}
			bp = GETPTR(NEXT_PTR(bp));
		}
	}

	//Are there any contiguous free blocks that somehow escaped coalescing?
	int j;
	for (j = 0; j < ELEMENTS; j++)
	{
		void* bp = free_list[j];
		while (bp != NULL)
		{
			void* prevblock = PREV_BLKP(bp);
			void* nextblock = NEXT_BLKP(bp);
			int prevblockalloc = GET_ALLOC(HDRP(prevblock));
			int nextblockalloc = GET_ALLOC(HDRP(nextblock));
			if (!prevblockalloc || !nextblockalloc)
			{
				printf("Escaped coalescing!\n");
			}
			bp = GETPTR(NEXT_PTR(bp));
		}
	}
	

	//check the linking of the free blocks in the free list has no problem
	int q;
	for (q = 0; q < ELEMENTS; q++)
	{
		void* check = free_list[i];
		while (check != NULL)
		{
			void * next = GETPTR(NEXT_PTR(check));
			if (next != NULL && GETPTR(PREV_PTR(next)) != check)
			{
				printf("Linking error in free list!\n");
				break;
			}
			check = GETPTR(NEXT_PTR(check));
		}
	}

	//check that the pointers point to valid heap addresses
	char *valid = heap_listp;
	while (GET_SIZE(HDRP(valid)) != 0)
	{
		if (valid < mem_heap_lo() && valid > mem_heap_hi)
		{
			printf("Not within range of heap!\n");
			break;
		}
		if ((GET_SIZE(HDRP(valid)) % 8) != 0)
		{
			printf("Block size not valid. It is not a multiple of 8!\n");
			break;
		}
		valid = NEXT_BLKP(valid);
	}

}

//helper functions for free_list
//insert function inserts in an increasing order of size into the free list
void insert_block(void* ptr, size_t size)
{
	int index = getindex(size);
	//doubly linked list insertion
	//cases to consider: empty list, 1 item list, more than 1 item list
	if (free_list[index] == NULL) //empty list
	{
		PUTPTR(NEXT_PTR(ptr), NULL); //set the next address that ptr points to to null
		PUTPTR(PREV_PTR(ptr), NULL); //set the previous address that the pointer points to to null 
		free_list[index] = ptr; //free list is now no longer empty
		return;
	}
	else
	{
		void *current = free_list[index]; //set pointer to the first element of the free list
		//insertion into head (always insert into head)
		PUTPTR(NEXT_PTR(ptr), current);
		PUTPTR(PREV_PTR(ptr), NULL);
		PUTPTR(PREV_PTR(current), ptr);
		free_list[index] = ptr;
	}
	return;
}


void delete_block(void* ptr, size_t size) /*delete function for deleting a free block from the free list*/
{
	int index = getindex(size);
	void* current = ptr;
	//special cases to consider when deleting
	//1 item list
	if (GETPTR(NEXT_PTR(current)) == NULL && GETPTR(PREV_PTR(current)) == NULL) //loop executed 1 time only and current's next is null, it means the list is a 1 item list
	{
		free_list[index] = NULL; //make free list an empty list after the deletion of 1 item
	}
	else //more than 1 items
	{
		//cases to consider
		//head, middle , tail
		if (GETPTR(NEXT_PTR(current)) == NULL) //deletion at the tail
		{
			void* currentprev = GETPTR(PREV_PTR(current));
			PUTPTR(NEXT_PTR(currentprev), NULL);
		}
		else if (GETPTR(PREV_PTR(current)) == NULL) //deletion at the head
		{
			void* currentnext = GETPTR(NEXT_PTR(current));
			PUTPTR(PREV_PTR(currentnext), NULL);
			free_list[index] = currentnext;
		}
		else //deletion in the middle
		{
			void* currentnext = GETPTR(NEXT_PTR(current));
			void* currentprev = GETPTR(PREV_PTR(current));
			PUTPTR(NEXT_PTR(currentprev), currentnext);
			PUTPTR(PREV_PTR(currentnext), currentprev);
		}
	}
	return;
}




/* helper function */
/* The remaining routines are internal helper routines */


/*
 * coalesce - boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) { /* Case 1 */
		return bp;
	}

	else if (prev_alloc && !next_alloc) { /* Case 2 */
		size_t sizenext = GET_SIZE(HDRP(NEXT_BLKP(bp)));
		//next block is unallocated
		//////////////////////////////////////////////////////
		delete_block(NEXT_BLKP(bp), sizenext);
		delete_block(bp, size);
		///////////////////////////////////////////////////////
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}

	else if (!prev_alloc && next_alloc) { /* Case 3 */
		size_t sizeprev = GET_SIZE(HDRP(PREV_BLKP(bp)));
		//prev block is unallocated
		///////////////////////////////////////////////////////////
		delete_block(PREV_BLKP(bp), sizeprev);
		delete_block(bp, size);
		///////////////////////////////////////////////////////////
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	else { /* Case 4 */
		size_t sizenext = GET_SIZE(HDRP(NEXT_BLKP(bp)));
		size_t sizeprev = GET_SIZE(HDRP(PREV_BLKP(bp)));
		//both next block and prev block unallocated
		//////////////////////////////////////////////////////
		delete_block(NEXT_BLKP(bp), sizenext);
		delete_block(PREV_BLKP(bp), sizeprev);
		delete_block(bp, size);
		////////////////////////////////////////////////////////
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
			GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	insert_block(bp, size);
	return bp;
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words)
{
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
	PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

	insert_block(bp, size);

	/* Coalesce if the previous block was free */
	return coalesce(bp);
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));

	if ((csize - asize) >= (3 * DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
		insert_block(bp, csize - asize); //new free block arising from place, insert it into the free list
	}
	else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}

/*
 * find_fit - Find a fit for a block with asize bytes
 */
static void *find_fit(size_t asize)
{
	int index = getindex(asize);
	/* First-fit search */
	void *bp;
	while (index < ELEMENTS)
	{
		for (bp = free_list[index]; bp != NULL; bp = bp = GETPTR(NEXT_PTR(bp))) {
			if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
				return bp;
			}
		}
		index++;
	}
	return NULL; /* No fit */
}




/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	int index;
	for (index = 0; index < ELEMENTS; index++)
	{
		free_list[index] = NULL;
	}
	/* Create the initial empty heap */
	if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
		return -1;
	PUT(heap_listp, 0); /* Alignment padding */
	PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
	PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
	PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); /* Epilogue header */
	heap_listp += (2 * WSIZE);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
		return -1;
	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t asize; /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	char *bp;
	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 3 * DSIZE;
	else
		asize = DSIZE * ((size + (2 * DSIZE) + (DSIZE - 1)) / DSIZE);
	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		delete_block(bp, GET_SIZE(HDRP(bp)));
		place(bp, asize);
		return bp;

	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
		return NULL;
	delete_block(bp, GET_SIZE(HDRP(bp)));
	place(bp, asize);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	insert_block(bp, GET_SIZE(HDRP(bp)));
	coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	//EDGE CASES
	//////////////////////////////////
	if (ptr == NULL)
	{
		return(mm_malloc(size));
	}
	if (size == 0)
	{
		mm_free(size);
		return NULL;
	}
	///////////////////////////////////
	size_t oldsize = GET_SIZE(HDRP(ptr));
	// Align block size
	size = ALIGN(size);

	//case if old size is equal to size
	if (oldsize == size)
	{
		return ptr;
	}
	//case when current block too small to fit new size
	else if (oldsize < size)
	{
		void* newptr = mm_malloc(size);
		void* oldptr = ptr;
		memcpy(newptr, oldptr, oldsize);
		mm_free(oldptr);
		return newptr;
	}
	else
	{
		void* bp = ptr;
		if ((oldsize - size) >= (3 * DSIZE)) {
			PUT(HDRP(bp), PACK(size, 1));
			PUT(FTRP(bp), PACK(size, 1));
			bp = NEXT_BLKP(bp);
			PUT(HDRP(bp), PACK(oldsize - size, 0));
			PUT(FTRP(bp), PACK(oldsize - size, 0));
			insert_block(bp, oldsize - size); //new free block arising from place, insert it into the free list
		}
		else {
			PUT(HDRP(bp), PACK(oldsize, 1));
			PUT(FTRP(bp), PACK(oldsize, 1));
		}
		return ptr;
	}
}



















