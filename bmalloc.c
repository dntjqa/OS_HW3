#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
	int size_field = 4;
	while((1 << size_field) < s){
		size_field++;
	}
	// 2의 거듭제곱 만큼 size_field 증가
	return size_field;
}

void* bmalloc(size_t s) {
    int size_field = fitting(s);
    size_t block_size = 1 << size_field;
    // 적합한 블록 사이즈 계산

    bm_header* itr = &bm_list_head;
    bm_header* prev = NULL;

    if (bm_mode == BestFit) {
        while (itr != NULL) {
            if (!itr->used && itr->size >= size_field) {
                break;
            }
            prev = itr;
            itr = itr->next;
        }
    } else if (bm_mode == FirstFit) {
        while (itr != NULL) {
            if (!itr->used && itr->size >= size_field) {
                break;
            }
            prev = itr;
            itr = itr->next;
        }
    }

    if (itr == NULL) {
        bm_header* new_page = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_page == MAP_FAILED) {
            return NULL; 
        }

        new_page->used = 0;
        new_page->size = 12;
        new_page->next = NULL;

        if (prev != NULL) {
            prev->next = new_page;
        } else {
            bm_list_head.next = new_page;
        }

        itr = new_page;
    }

    while (itr->size > size_field) {
        
        bm_header* new_block = (bm_header*)((char*)itr + sizeof(bm_header) + (block_size >> 1));
        new_block->used = 0;
        new_block->size = itr->size - 1;
        new_block->next = itr->next;

        itr->used = 1;
        itr->size -= 1;
        itr->next = new_block;
    }

    itr->used = 1; 
    return ((char*)itr) + sizeof(bm_header);
}


void bfree (void * p) 
{
	// TODO 
	bm_header* header = (bm_header*)((char*)p - sizeof(bm_header));
    header->used = 0;

    // Coalesce blocks if possible
    bm_header* itr = &bm_list_head;
    while (itr->next != NULL){
        if (!itr->next->used && !header->used && itr->next->size == header->size){
            itr->next = itr->next->next;
            itr->size++;
            header = itr;
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

void* brealloc(void* p, size_t s) {
    if (p == NULL) {
        return bmalloc(s);
    } // p가 NULL인 경우 새로운 메모리 블록 할당

    bm_header* old_header = (bm_header*)((char*)p - sizeof(bm_header));
    size_t old_size = 1 << old_header->size;

    if (s <= old_size) {
        return p;
    } // 요청된 크기보다 작으면 p return

    void* new_block = bmalloc(s);
    if (new_block == NULL) {
        return NULL;
    }
    memcpy(new_block, p, old_size);
    bfree(p);

    return new_block;
}


void bmconfig (bm_option opt) 
{
	// TODO
	bm_mode = opt;
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
    int internal_fragmentation = 0;

    for(itr = bm_list_head.next; itr != 0x0; itr = itr->next){
        total_memory += (1 << itr->size);
        if(itr->used){
            used_memory += (1 << itr->size);
        }
        else{
            available_memory += (1 << itr->size);
            internal_fragmentation += (1 << itr->size) - sizeof(bm_header);
        }
    }

    printf("Total Memory: %d\n", total_memory);
    printf("Used Memory: %d\n", used_memory);
    printf("Available Memory: %d\n", available_memory);
    printf("Internal Fragmentation: %d\n", internal_fragmentation);
}
