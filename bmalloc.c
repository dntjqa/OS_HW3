#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "bmalloc.h" 

bm_option bm_mode = BestFit ;
bm_header bm_list_head = {0, 0, 0x0 } ;

void * sibling (void * h)
{
	// TODO
	bm_header* header = (bm_header*)h; // casting
	size_t block_size = 1 << header->size;
	// get the size of block
	return ((char*)header) + sizeof(bm_header) + block_size;
	// 현재 블록의 헤더와 다음 블록의 헤더 건너뛰기
	// 헤더 주소에서 의심되는 형제 블록의 헤더 주소 반환
}

int fitting (size_t s) 
{
	// TODO
	int size_field = 0;
	while((1 << size_field) < s){
		size_field++;
	}
	// 2의 거듭제곱 만큼 size_field 증가
	return size_field;
}

void * bmalloc (size_t s)
{
	int size_field = fitting(s);
	// get the size of s
    bm_header *itr = &bm_list_head;
	// linked list 순회용 포인터
    bm_header *selected_block = NULL;
	// 적합한 메모리 블록을 가리키기 위한 포인터

    if (bm_mode == BestFit){ // BestFit일 때
        // Find the smallest block that is larger than the required size
        while (itr->next != NULL){
            if (itr->next->size >= size_field){
				// size가 주어진 s보다 크거나 같을 때
                if (selected_block == NULL || itr->next->size < selected_block->size){
                    selected_block = itr->next;
                }
            }
            itr = itr->next;
        }
    }
    else if (bm_mode == FirstFit){// FirstFit일 때
        // Find the first block that is large enough
        while (itr->next != NULL){
            if (itr->next->size >= size_field){
                selected_block = itr->next;
                break;
            }
            itr = itr->next;
        }
    }

    if (selected_block != NULL){
        // Split the block if necessary
        if (selected_block->size > size_field){
            size_t block_size = 1 << size_field;
            bm_header *new_block = (bm_header *)((char *)selected_block + sizeof(bm_header) + block_size);
            new_block->used = 0;
            new_block->size = selected_block->size - size_field - 1;
            new_block->next = selected_block->next;
            selected_block->next = new_block;
            selected_block->size = size_field;
        }

        selected_block->used = 1; // Mark the block as used
        return ((char *)selected_block) + sizeof(bm_header);
    }

    // No suitable block found, return NULL
	return NULL;
}

void bfree (void * p) 
{
	// TODO 
	bm_header* header = (bm_header*)((char*)p - sizeof(bm_header));
    header->used = 0;

    // Coalesce blocks if possible
    bm_header* itr = &bm_list_head;
    while (itr->next != NULL){
        if (!itr->next->used && itr->next->next != NULL && !itr->next->next->used
			&& itr->next->size == itr->next->next->size){
            itr->next = itr->next->next;
            itr->size++;
        }
        else{
            itr = itr->next;
        }
    }

    // Unmap the entire page if no blocks are used
    if (bm_list_head.next == NULL){
        munmap(&bm_list_head, 4096);
    }
}

void * brealloc (void * p, size_t s) 
{
	// TODO
	// Allocate a new block with the requested size
    void* new_block = bmalloc(s);
    if (new_block == NULL){
        return NULL;
    }

    // Copy the data from the old block to the new block
    size_t block_size = 1 << fitting(s);
    bm_header* old_header = (bm_header*)((char*)p - sizeof(bm_header));
    size_t copy_size = (block_size < old_header->size) ? block_size : old_header->size;
    memcpy(new_block, p, copy_size);

    // Free the old block
    bfree(p);

    return new_block;
}

void bmconfig (bm_option opt) 
{
	// TODO
	bm_mode = opt;
}


void 
bmprint () 
{
	bm_header_ptr itr ;
	int i ;

	printf("==================== bm_list ====================\n") ;
	for (itr = bm_list_head.next, i = 0 ; itr != 0x0 ; itr = itr->next, i++) {
		printf("%3d:%p:%1d %8d:", i, ((void *) itr) + sizeof(bm_header), (int)itr->used, (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(bm_header) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++) 
			printf("%02x ", s[j]) ;
		printf("\n") ;
	}
	printf("=================================================\n") ;

	//TODO: print out the stat's.
	size_t total_memory = 0;
	size_t used_memory = 0;
	size_t available_memory = 0;
	size_t internal_fragmentation = 0;

	for(itr = bm_list_head.next; itr != NULL; itr = itr->next){
		total_memory += 1 << itr->size;
		if(itr->used){
			used_memory += 1 << itr->size;
		}
		else{
			available_memory += 1 << itr->size;
			internal_fragmentation += (1 << itr->size) - sizeof(bm_header);
		}
	}
	printf("Total Memory: %lu bytes\n", total_memory);
	printf("Used Memory: %lu bytes\n", used_memory);
	printf("Available Memory: %lu bytes\n", available_memory);
	printf("Internal Fragmentation: %lu bytes\n", internal_fragmentation);
}
