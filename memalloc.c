#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
    // Allocate memory for the memory pool using mmap.
	pool1 = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if(pool1 == MAP_FAILED) {
		fprintf(stderr, "mmap failed\n");
		exit(EXIT_FAILURE);  // Exit on error
	}

    // Set the starting address for usable memory after the memPool header.
    pool1->start = (char *)pool1 + sizeof(memPool);
    pool1->size = PAGE_SIZE;

    // Initialize the first memory chunk in the pool.
    pool1->firstChunk.next = NULL;
    pool1->firstChunk.size = PAGE_SIZE - sizeof(memChunk);
    pool1->firstChunk.isFree = 1;

    return (void *)((char *)pool1->start); // Return the pointer to the start of the usable memory.
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

    // Validate the requested size.
    if(size == 0 || size > PAGE_SIZE){
        fprintf(stderr, "Invalid size passed");
        exit(EXIT_FAILURE);  // Exit on error
    }

    // Initialize the memory pool if it doesn't exist.
    if(pool1==NULL)
        initialize_pool();

    memChunk *currChunk = &pool1->firstChunk;

    // Loop through the chunks to find a suitable free chunk.
    while(currChunk){
        // Check if the current chunk is free and large enough.
        if(currChunk->isFree && currChunk->size >= size){
            currChunk->isFree = 0;

            // If there is enough space, split the chunk.
            if(currChunk->size > size + sizeof(memChunk)){
                // Create a new chunk for the remaining space.
                memChunk *nextChunk = (memChunk *)((char *)currChunk + sizeof(memChunk) + size);
                nextChunk->next = currChunk->next;
                nextChunk->size = currChunk->size - size - sizeof(memChunk);
                nextChunk->isFree = 1;

                currChunk->next = nextChunk;
                currChunk->size = size;
            }

            // Return a pointer to the allocated memory, which is just after the chunk header.
            return (void *)((char *)currChunk + sizeof(memChunk));
        }
        currChunk = currChunk->next;// Move to the next chunk in the pool.
    }

    //Free chunk with requested size not found.
    fprintf(stderr, "Not enough memory :(\n");
    exit(EXIT_FAILURE);  // Exit on error    
}

/*

mem_free function is freeing the requested chunk if the chunk in the memory pool.
The function check 2 possible scenarios: 
First, if the next chunk is free.
Second, if the previus chunks is free.
If one of this conditions is true, it will merge the chunks together.

** Arguments:
   - The function takes a undecleared type pointer.
** Return:
   - No returns.

*/
void mem_free(void *ptr){
    if(!ptr) return;

    // Set a pointer to the beginning of the chunk header.
    memChunk *chunk = (memChunk *)((char *)ptr - sizeof(memChunk));

    //Validate if the chunk in the memory pool.
    if(chunk < &pool1->firstChunk || (char *)chunk >= (char *)pool1 + pool1->size){
        fprintf(stderr, "Invalid pointer passed to mem_free\n");
        exit(EXIT_FAILURE);  // Exit on error
    }

    // If the chunk is already free
    if(!chunk->isFree) chunk->isFree = 1;
    else{
        fprintf(stderr, "Trying to free an already free memory");
        exit(EXIT_FAILURE);  // Exit on error
    }

    memChunk *tempChunk = &pool1->firstChunk;

    while (tempChunk) {
        // Check if the next chunk exists.
        if (tempChunk->next && tempChunk->next->isFree) {
            // Merge with the next chunk.
            tempChunk->size += sizeof(memChunk) + tempChunk->next->size;
            tempChunk->next = tempChunk->next->next; // Remove the next chunk from the list.
        } else {
            // Move to the next chunk.
            tempChunk = tempChunk->next;
        }
    }
}

//Example code
int main(){
    void* a = (int *)mem_alloc(sizeof(int));  

    printf("The address of a: %p\n", a);

    mem_free(a);

    return 0;
}