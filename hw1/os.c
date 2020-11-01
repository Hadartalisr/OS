
#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <err.h>
//#include "os.hSSS"
#include "pt.c"


/*2^20 pages ought to be enough for anybody */
#define NPAGES	(1024*1024)

static void* pages[NPAGES];


uint64_t alloc_page_frame(void)
{
	static uint64_t nalloc;
	uint64_t ppn;
	void* va;

	if (nalloc == NPAGES)
		errx(1, "out of physical mem-ory");

	/* OS memory management isn't really this simple */
	ppn = nalloc;
	nalloc++;

	va = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (va == MAP_FAILED)
		err(1, "mmap failed");

	pages[ppn] = va;	
	return ppn;
}

void* phys_to_virt(uint64_t phys_addr)
{
	uint64_t ppn = phys_addr >> 12;
	uint64_t off = phys_addr & 0xfff;
	void* va = NULL;
	if (ppn < NPAGES)
		va = pages[ppn] + off;
	return va;
}


int main(int argc, char **argv)
{
	uint64_t pt =  allocate_frame();
	print_int64_value(pt);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	
	page_table_update(pt, 0xcafe, 0xf00d);
	assert(page_table_query(pt, 0xcafe) == 0xf00d);
	
	/*
	page_table_update(pt, 0xcafe, 0xf00d);
	assert(page_table_query(pt, 0xcafe) == 0xf00d);
	page_table_update(pt, 0xcafe, NO_MAPPING);
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);new_addre
	*/
	printf("\n\n Finished successfully! \n");
	return 0;
}

	/*for(int i = 1; i < 10; i++){
		pt =  allocate_frame();
		print_int64_value(pt);
		uint64_t new_address = pt;
		uint64_t* p;
		for(int j =0; j < 10; j++){
			p = phys_to_virt(new_address+j*4);
			print_int64_value(p);
		}
		printf("\n");
	}*/
