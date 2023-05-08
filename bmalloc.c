#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include "bmalloc.h" 


bm_option bm_mode = BestFit ;
bm_header bm_list_head = {0, 0, 0x0 } ;

void * sibling (void * h)
{
	// TODO
	return ((bm_header_ptr) h)->next;
}

int fitting (size_t s) 
{
    int ret = -1; // If no space is found, return -1.
    bm_header_ptr itr = bm_list_head.next;

    while (itr != NULL)
    {
        if (!itr->used && itr->size >= s)
        {
            if (bm_mode == BestFit)
            {
                if (ret == -1 || itr->size < ((bm_header_ptr)ret)->size)
                    ret = (int)itr;
            }
            else if (bm_mode == FirstFit)
            {
                ret = (int)itr;
                break;
            }
        }
        itr = itr->next;
    }

    return ret;
}

void * bmalloc (size_t s) 
{
bm_header_ptr new_block = NULL;
    void* ret = NULL;

    // Find a free block with size greater than or equal to s.
    int free_block = fitting(s);

    // If no block is found, allocate a new block using mmap().
    if (free_block == -1)
    {
        size_t alloc_size = sizeof(bm_header) + s;
        void* ptr = mmap(NULL, alloc_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        if (ptr == MAP_FAILED)
            return NULL;

        new_block = (bm_header_ptr)ptr;
        new_block->used = 1;
        new_block->size = s;
        new_block->next = NULL;

        // Add the new block to the end of the block list.
        bm_header_ptr itr = &bm_list_head;
        while (itr->next != NULL)
            itr = itr->next;
        itr->next = new_block;

        ret = (void*)(new_block + 1);
    }
    else
    {
        // If a free block is found, use it.
        bm_header_ptr block = (bm_header_ptr)free_block;
        block->used = 1;

        // If the size of the block is greater than s plus the size of header,
        // split the block into two blocks.
        if (block->size > s + sizeof(bm_header))
        {
            bm_header_ptr new_block = (bm_header_ptr)((void*)block + sizeof(bm_header) + s);
            new_block->used = 0;
            new_block->size = block->size - s - sizeof(bm_header);
            new_block->next = block->next;

            block->size = s;
            block->next = new_block;
        }

        ret = (void*)(block + 1);
    }

    return ret;
}

void bfree (void * p) 
{
    if (p == NULL) return;

    // Free the allocated buffer starting at pointer p.
    bm_header_ptr hdr = (bm_header_ptr) p - 1;
    hdr->used = 0;

    // Coalesce consecutive free buffers into a single buffer.
    bm_header_ptr itr = &bm_list_head;
    while (itr->next != NULL) {
        bm_header_ptr next = itr->next;
        if (!next->used && !hdr->used && ((char *) hdr) + sizeof(bm_header) + hdr->size == (char *) next) {
            hdr->size += sizeof(bm_header) + next->size;
            hdr->next = next->next;
        } else {
            itr = itr->next;
        }
    }
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