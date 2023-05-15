#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bmalloc.h" 

bm_option bm_mode = BestFit ;
bm_header bm_list_head = {0, 0, 0x0 } ;

bm_header_ptr left_sibling ; // left
bm_header_ptr right_sibling ; // right

void * sibling (void * h)
{
	// TODO
    int size = ((bm_header *)h)->size ;

	bm_header_ptr itr ;
	bm_header_ptr left = &bm_list_head ;
	int block_size = 1 ; // size of block

	for (itr = bm_list_head.next ; itr != (bm_header *)h ; itr = itr->next) {
		if (itr == 0x0) {
			return NULL;
		}
		block_size += (1 << itr->size) ;
		left_sibling = left ;
		left = itr ;
	}
	
	if ((block_size / size) % 2 == 0) {
		left_sibling = left ;
		right_sibling = ((bm_header *)h)->next->next ;
		return ((bm_header *)h)->next ; 
	} // left block
	else {	
		right_sibling = ((bm_header *)h)->next ;
		return left ;
	} // right block
}

int fitting (size_t s) 
{	
	// TODO
	int size_field = 1;
	while((1 << size_field) < s){
		size_field++;
	}
    if(size_field < 4 || size_field > 12)
        return 0;
    return size_field;
}

void * bmalloc (size_t s) 
{   
	bm_header_ptr itr ;
	bm_header_ptr bm_list_tail ;
	int size_field = fitting(s) ;

	if (bm_list_head.next == 0x0) {
        itr = mmap(NULL, 4096,  PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS , -1, 0);
		if (itr == MAP_FAILED) {
			return NULL;
		} 
		
		bm_list_head.next = itr ;
		itr->used = 0 ;
		itr->size = 12 ;
		itr->next = 0x0 ;
	} // Init the first block

	bm_header_ptr fitting_block = malloc(sizeof(bm_header)) ; 
	fitting_block->size = 13 ;
    // To search minimum block
	
	for (itr = bm_list_head.next ; itr != 0x0 ; itr = itr->next) {
		if (size_field <= itr->size && itr->used == 0) {
			if (bm_mode == FirstFit) {
				fitting_block = itr ;
				break ;
			}
			else if (itr->size < fitting_block->size) {
				fitting_block = itr ;
			} // BestFit
		}
		if (itr->next == 0x0) {
			bm_list_tail = itr ;
		}
	}

	if (12 < fitting_block->size) {
		itr = mmap(NULL, 4096,  PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS , -1, 0);
		if (itr == MAP_FAILED) {
			return NULL;
		} 
		bm_list_tail->next = itr ;
		itr->used = 0 ;
		itr->size = 12 ;
		itr->next = 0x0 ;
		fitting_block = itr ;
	}
    while(size_field < fitting_block->size){
        bm_header_ptr fitting_next = fitting_block->next ;

		fitting_block->next = (bm_header_ptr) ((char *)fitting_block + (1 << (fitting_block->size - 1))) ;
		fitting_block->next->used = 0 ;
		fitting_block->next->size = fitting_block->size - 1 ;
		fitting_block->next->next = fitting_next ;

        fitting_block->size = fitting_block->size - 1;
    }
	fitting_block->used = 1 ;
    
	void *payload = ((bm_header_ptr)((char *)fitting_block + sizeof(bm_header)));
    memset(payload, 0, (1 << fitting_block->size) - sizeof(bm_header));
    // Init payload
    return payload;
}

void bfree (void * p) 
{
	// TODO
	p = ((bm_header_ptr) ((char *)p - sizeof(bm_header))) ;
	((bm_header *)p)->used = 0 ;

	bm_header_ptr itr ;
	for (itr = p ; ((bm_header *)sibling(itr))->used == 0 ; itr = left_sibling->next) {
		if (itr->size == ((bm_header *)sibling(itr))->size) { 
			left_sibling->next->next = right_sibling ;
			left_sibling->next->size++ ;
		}
		else 
			break ;

		if (left_sibling->next->size == 12){
            if (munmap(left_sibling->next, 4096) == -1) {
                return;
		    }
            left_sibling->next = right_sibling ;
            
            break;
        }
	}
}

void * brealloc (void * p, size_t s) 
{
	bfree(p) ;
	return bmalloc(s) ;
}

void bmconfig (bm_option opt) 
{   
    // TODO
	bm_mode = opt ;
}

void bmprint () 
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
    int total_memory = 0;
    int used_memory = 0;
    int available_memory = 0;
    int total_internal_fragmentation = 0;
    
    for(itr = bm_list_head.next; itr != 0x0; itr = itr->next){
        int allocated_size = 1 << itr->size;
        int requested_size = allocated_size - sizeof(bm_header);
        int internal_fragmentation = allocated_size - requested_size;

        if (itr->used == 0) // Not used block
        {
            available_memory += requested_size;
            total_internal_fragmentation += internal_fragmentation;
        }
        else // Used block
        {
            used_memory += requested_size;
        }
        total_memory += requested_size;
    }

    printf("Total Memory: %d\n", total_memory);
    printf("Used Memory: %d\n", used_memory);
    printf("Available Memory: %d\n", available_memory);
    printf("Internal fragmentation: %d\n", total_internal_fragmentation);
}
