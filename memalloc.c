#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096 // Assuming a page size of 4096 bytes

//Single chunk struct
typedef struct memChunk {
    struct memChunk *next; // A pointer to the next chunk.
	size_t size;
    bool isFree;
} memChunk;

//Memory pool structure
typedef struct {
    void *start;
    size_t size;
    memChunk firstChunk;
} memPool;

memPool *pool1 = NULL;

/*

pool_alloc function is allocating a memory "pool", aka page (4096 bytes) or block,
and creating the first chunk from that allocated memory.

** Arguments: 
   - no arguments.
** Return:
   - returning a pointer to the beginning of thr memory pool.

*/
void* initialize_pool(){
	pool1 = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if(pool1 == MAP_FAILED) {
		perror("mmap failed\n");
		return NULL;
	}

    pool1->start = (char *)pool1 + sizeof(memPool);
    pool1->size = PAGE_SIZE;

    pool1->firstChunk.next = NULL;
    pool1->firstChunk.size = PAGE_SIZE - sizeof(memChunk);
    pool1->firstChunk.isFree = 1;

    return (void *)((char *)pool1->start);
}

/*
mem_alloc allocates a chunk of memory from the available pool.
If the pool doesn't exist, it initializes it first.
The function searches for a free chunk that is large enough.
 
** Arguments: 
   - size_t size: The size of memory requested in bytes.
** Return: 
   - A pointer to the beginning of the allocated chunk, or NULL if allocation fails.
*/
void* mem_alloc(size_t size){
    // Initialize the memory pool if it doesn't exist.
    if(pool1==NULL){
        if(!initialize_pool()){
            printf("Error initialize pool\n");
            return NULL;
        }
    }
    memChunk *currChunk = &pool1->firstChunk;

    //Loop through the chunks.
    while(currChunk){
        if(currChunk->isFree && currChunk->size >= size){
            currChunk->isFree = 0;

            if(currChunk->size > size + sizeof(memChunk)){
                memChunk *nextChunk = (memChunk *)((char *)currChunk + sizeof(memChunk) + size);
                nextChunk->next = currChunk->next;
                nextChunk->size = currChunk->size - size - sizeof(memChunk);
                nextChunk->isFree = 1;

                currChunk->next = nextChunk;
                currChunk->size = size;
            }
            return (void *)((char *)currChunk + sizeof(memChunk));
        }
        currChunk = currChunk->next;
    }
    //Free chunk with requested size not found.
    fprintf(stderr, "Not enough memory :(\n");
    return NULL;    
}

/*

mem_free function is freeing the requested chunk if the chunk in the memory pool.
The function check 2 possible scenarios: First, if the next chunk is free.
Second, if the previus chunks is free.
If one of this conditions is true, it will merge the chunks together.

** Arguments:
   - The function takes a undecleared type pointer.
** Return:
   - No returns.

*/
void mem_free(void *ptr){
    if(!ptr) return;

    //Pointer to the beggining of the chunk header.
    memChunk *chunk = (memChunk *)((char *)ptr - sizeof(memChunk));

    //Check if the chunk in the memory pool.
    if(chunk < &pool1->firstChunk || (char *)chunk >= (char *)pool1 + pool1->size){
        fprintf(stderr, "Invalid pointer passed to mem_free\n");
        return;
    }

    chunk->isFree = 1;

    if(chunk){
        memChunk *tempChunk = chunk;
        while(tempChunk->next != NULL){

            if(tempChunk->next->isFree && tempChunk->isFree){
                printf("Freeing two chunks yehhh\n");
                tempChunk->size += sizeof(memChunk) + chunk->next->size;
                tempChunk->next = tempChunk->next->next;
                printf("The chunk starts from: %p\n", ((char *)tempChunk + sizeof(memChunk)));
            }
            if(tempChunk->next == NULL){
                printf("Start again :(\n");
                tempChunk = &pool1->firstChunk;
            }else{
                printf("Moving from %p to %p\n", ((char *)tempChunk + sizeof(memChunk)), ((char *)tempChunk->next + sizeof(memChunk)));
                tempChunk = tempChunk->next;
            }
        }
    }
}

int main(){
    
    //Example

    void* a = (int *)mem_alloc(sizeof(int));
    *((int *)a) = 8;

    printf("The value of a is: %d\n", *((int *)a));

    return 0;
}