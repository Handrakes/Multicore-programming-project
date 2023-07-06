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
    /* Your student ID */
    "20191611",
    /* Your full name*/
    "Jongseon Yoo",
    /* Your email address */
    "ironman8664@naver.com",
};


/* single word(4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
/* 할당할 크기 size를 확인하고 8의 배수로 할당하기 위함 */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
기본 상수와 매크로
*/

// Basic constants and macors
#define WSIZE       4           // Word and header/footer size(bytes)
#define DSIZE       8           // Double word size (btyes)
#define CHUNKSIZE   (1 << 12)   // Extend heap by this amount (bytes) 

#define MAX(x, y)   ((x) > (y) ? (x) : (y))    // x > y가 참이면 x, 거짓이면 y

// PACK : 크기와 할당 비트를 통합해서 header와 footer에 저장할 수 있는 값 리턴
#define PACK(size, alloc)   ((size) | (alloc))

// Read and wirte a word at address
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

// Read the size and allocated field from address p
#define GET_SIZE(p)    (GET(p) & ~0x7)  // header or footer의 사이즈 반환(8의 배수)
#define GET_ALLOC(p)   (GET(p) & 0x1)   // 현재 블록 가용 여부 판단(0이면 alloc, 1이면 free)

// bp(현재 블록의 포인터)로 현재 블록의 header 위치와 footer 위치 반환
#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 다음과 이전 블록의 포인터 반환
#define NEXT_BLKP(bp)   (((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)))    // 다음 블록 bp 위치 반환(bp + 현재 블록의 크기)
#define PREV_BLKP(bp)   (((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)))    // 이전 블록 bp 위치 반환(bp - 이전 블록의 크기)

// explicit 구현을 위한 pre + pos 포인터 변환
#define PRE_LINK(bp) (*(char **)(bp))
#define POS_LINK(bp) (*(char **)(bp + WSIZE))


// Declaration
static void *heap_listp;
static void *free_listp;

static void add_free(void *bp); // 새로운 공간을 추가하는 함수
static void remove_free(void *bp); // 새로운 공간을 지우는 함수

static void *extend_heap(size_t words);
static void *coalesce(void *bp); // 병합
static void *find_fit(size_t a_size);
static void place(void *bp, size_t a_size);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // Create the initial empty heap
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void *)-1) {  // heap_listp가 힙의 최댓값 이상을 요청한다면 fail
        return -1;
    }

    PUT(heap_listp, 0);                             // Alignment padding
    PUT(heap_listp + (1*WSIZE), PACK(2 * DSIZE, 1));    // Prologue header
    
    PUT(heap_listp + (2*WSIZE), (int)NULL); // pre 공간 for explicit
    PUT(heap_listp + (3*WSIZE), (int)NULL); // pos 공간 for explicit
    
    PUT(heap_listp + (4*WSIZE), PACK(2 * DSIZE, 1));    // Prologue footer
    PUT(heap_listp + (5*WSIZE), PACK(0, 1));        // Epilogue header
    
    free_listp = heap_listp + DSIZE; // 앞의 alignment와 prologue를 더한 8 byte 뒤의 주소값

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    // Allocate an even number of words  to maintain alignment
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; // words가 홀수면 +1을 해서 공간 할당
    if ((long)(bp = mem_sbrk(size)) == -1) { //sbrk로 공간확장
        return NULL;
    }

    // initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // coalesce if the previous block was free
    return coalesce(bp);   
}

///////////////////////////////////////////////////////////////////////
static void add_free(void *bp){ //free block 은 가장 앞으로 들어간다.
    POS_LINK(bp) = free_listp;
    PRE_LINK(free_listp) = bp;
    PRE_LINK(bp) = NULL;
    free_listp = bp;

}

static void remove_free(void *bp){ // 포인터 연결해주기 
    // 가장 앞에서 삭제
    if(free_listp == bp){
        PRE_LINK(POS_LINK(bp)) = NULL;
        free_listp = POS_LINK(bp);
    }

    // 앞을 제외한 곳에서 삭제
    else{
        POS_LINK(PRE_LINK(bp)) = POS_LINK(bp);
        PRE_LINK(POS_LINK(bp)) = PRE_LINK(bp);
    }
}

///////////////////////////////////////////////////////////////////////

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case1: 앞, 뒤 블록 모두 할당되어 있을 때
    if (prev_alloc && next_alloc) {
        add_free(bp);
        return bp;
    }

    // case2: 앞 블록 할당, 뒷 블록 가용
    else if (prev_alloc && !next_alloc) {
        remove_free(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        add_free(bp);
    }

    // case3: 앞 블록 가용, 뒷 블록 할당
    else if (!prev_alloc && next_alloc) {
        remove_free(PREV_BLKP(bp));
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

        add_free(bp);
    }

    // case4: 앞, 뒤 블록 모두 가용
    else {
        remove_free(NEXT_BLKP(bp));
        remove_free(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

        add_free(bp);
    }
    return bp;
} 


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t a_size;       // adjusted block szie
    size_t extend_size;  // Amount to extend heap if no fit
    char *bp;

    // Ignore spurious requests
    if (size == 0) {
        return NULL;
    }

    // Adjust block size to include overhead and alignment reqs
    if (size <= DSIZE) {    // 2words 이하의 사이즈는 4워드로 할당
        a_size = 2 * DSIZE;
    }
    else {                  // 할당 요청의 용량이 2words 초과 시, 8byte의 배수의 용량 할당
        a_size = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }

    // Search the free list for a fit
    if ((bp = find_fit(a_size)) != NULL) {   // 빈 블록 검색
        place(bp, a_size);                   
        return bp;
    }

    // NO fit found. Get more memory and place the block
    extend_size = MAX(a_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size/WSIZE)) == NULL) {   
        return NULL;
    }
    place(bp, a_size);
    return bp;
}

static void *find_fit(size_t a_size) {
    void *bp;

    for(bp = free_listp ; GET_ALLOC(HDRP(bp)) != 1 ; bp = POS_LINK(bp)){
        if(GET_SIZE(HDRP(bp)) >= a_size)
            return bp;
    }
    return NULL;    // NO fit
}


static void place(void *bp, size_t a_size) {
    size_t c_size = GET_SIZE(HDRP(bp));

    remove_free(bp);
    if ((c_size - a_size) >= (2 * (DSIZE))) {
        // a_size 만큼 용량 더하기
        PUT(HDRP(bp), PACK(a_size, 1));
        PUT(FTRP(bp), PACK(a_size, 1));
        
        bp = NEXT_BLKP(bp);
        // header, footer 배치
        PUT(HDRP(bp), PACK(c_size - a_size, 0));
        PUT(FTRP(bp), PACK(c_size - a_size, 0));

        add_free(bp);
    }
    else {  // c_size와 a_szie 차이가 네 칸(16byte)보다 작다면 해당 블록 통째로 사용
        PUT(HDRP(bp), PACK(c_size, 1));
        PUT(FTRP(bp), PACK(c_size, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *old_bp = bp;
    void *new_bp;
    size_t copySize;
    
    new_bp = mm_malloc(size);
    if (new_bp == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(old_bp));
    if (size < copySize)
      copySize = size;
    memcpy(new_bp, old_bp, copySize);
    mm_free(old_bp);
    return new_bp;
}