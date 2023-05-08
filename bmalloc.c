#include <unistd.h>
#include <stdio.h>
#include "bmalloc.h" 

bm_option bm_mode = BestFit ;
bm_header bm_list_head = {0, 0, 0x0 } ;

void * sibling (void * h)
{
	// TODO
}

int fitting (size_t s) 
{
	// TODO
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
}

void * brealloc (void * p, size_t s) 
{
	// TODO
	return 0x0 ; // erase this 
}

void bmconfig (bm_option opt) 
{
	// TODO
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
}

