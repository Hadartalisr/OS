#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "os.h" /* The headers which i will implement in the assignment */


#define num_of_levels 5
int VPN_indices[7][2] = {
		{0,11}, // least important index - VPNIndices[0] , offset |12|
		{12,20}, // last level entry |9|
		{21,29}, // |9|
		{30,38}, // |9|
		{39,47}, // |9|
		{48,56}, // first level entry |9|
		{57,63} // most important index - VPNIndices[6] , sign extend of 56_th bit |7|
};
int PPN_indices[3][2] = {
		{0,0}, //valid bit |1|
		{1,11}, // zeros |11|
		{12,63} // page frame address |52|
};


/**
 *	The method prints the value of uint64_t.
 */
void print_int64_value(uint64_t i){
	printf("%" PRIu64 "\n", i);
	return;
}


/**
 *	The method prints the binary representation of uint64_t.
 */
void print_binary(uint64_t number){
    if (number >> 1) {
        print_binary(number >> 1);
    }
    putc((number & 1) ? '1' : '0', stdout);
}

/**
 * The method allocates new page frame and returns the physical address of the new frame.
 */
uint64_t allocate_frame(){
	uint64_t ret = alloc_page_frame();
	ret = ret << 12;
	return ret;
}


/**
 * The method gets a vpn and level (0-4),
 * returns the index in the level's corresponding page table.
 */
uint64_t get_vpn_index(uint64_t vpn, int level){
	printf("\nFUNC get_vpn_index:\n");
	printf("vpn :"); print_int64_value(vpn); 
	printf("level :%d\n", level);
	int offset = 7 + (level*9);
	uint64_t index = (vpn << offset) >> 55;
	/* move left that the first 9 digits will be the relevant ones
	 * and 55 to the right so at the the end the number represents the number of offset bytes */
	printf("FUNC get_vpn_index RETURN: "); print_int64_value(index);
	return index;
}


/**
 * Tester for get_vpn_index.
 */
void vpn_test(uint64_t vpn){
	printf("Func vpn test, vpn : ");print_int64_value(vpn);
	print_binary(vpn);printf("\n");
	for(int level = 0 ; level < num_of_levels ; level ++){
		printf("level %d:\n", level);
		print_int64_value(get_vpn_index(vpn, level));
		printf("\n");
	}
}


/**
 * The method gets ppn and returns if its valid bit = 1
 */
int is_entry_valid(uint64_t ppn){
	printf("\nFUNC is_entry_valid:\n");
	printf("ppn: "); print_int64_value(ppn);	
	int ret = ppn & 0x1;
	printf("FUNC is_entry_valid RETURN : %d\n", ret);
	return ret;
}


/**
 *	The method gets
 *		pt_num - virtal page table base ADDRESS
 *		pt_level - the level of the page table, integer between 0-4
 *		vpn - to search for
 *	returns 1 if entry exists,
 *		and changes the next_pt_base_address to be the virtual addrees of the new page table
 */
int is_valid_entry_exist(uint64_t pt_base_address, int pt_level ,uint64_t vpn, uint64_t* next_pt_base_address){
	printf("\nFUNC is_valid_entry_exist:\n");
	printf("pt_base_address: "); print_int64_value(pt_base_address);
	printf("pt_level: "); print_int64_value(pt_level);
	printf("vpn: "); print_int64_value(vpn);

	int ret = 0;
	uint64_t index = get_vpn_index(vpn, pt_level);
	uint64_t index_pointer_address = pt_base_address+index;;
	printf("address of index in the table:"); print_int64_value(index_pointer_address); 

	uint64_t index_pointer_val = *((uint64_t *)index_pointer_address);
	printf("index_pointer_val:"); print_int64_value(index_pointer_val); 
	printf("index_pointer_binary_val:");print_binary(index_pointer_val);
	if(is_entry_valid(index_pointer_val)){
		ret = 1;
	}
	else {
		ret = 0;
	}
	*next_pt_base_address = (index_pointer_val >> 1) << 1 ;
	printf("FUNC is_valid_entry_exist RETURN : %d\n", ret);
	return ret;
}


uint64_t get_vpn_offset(uint64_t vpn){
	return ((vpn << 52) >> 52);
}


/**
 *	The method gets -
 *		pt - base address of the page table
 *		vpn - to search for
 *	returns uint64_t ppn - physical page number that the vpn is mapped to 
 * 		or NO_MAPPING if no mapping exists.
 */
uint64_t page_table_query(uint64_t pt, uint64_t vpn){
	printf("\nFUNC page_table_query:\n");
	printf("pt: "); print_int64_value(pt);
	printf("vpn: "); print_int64_value(vpn);

	pt = pt << 12;
	uint64_t pt_base_address = (uint64_t)phys_to_virt(pt);
	uint64_t* next_pt_base_address = (uint64_t*)calloc(1, sizeof(uint64_t*));
	int is_valid_entry = 1;
	int level = 0;
	uint64_t ret = NO_MAPPING;
	for(; (level <= num_of_levels-1) && is_valid_entry ; level++){
		is_valid_entry = is_valid_entry_exist(pt_base_address, level, vpn, next_pt_base_address);
		pt_base_address = *next_pt_base_address ;
	}
	if(is_valid_entry){ // the 5_th level 
		printf("\n~~~ 5_th level ~~~\n");
		printf("pt_base_address: "); print_int64_value(pt_base_address);
		printf("pt_level: "); print_int64_value(level);
		printf("vpn: "); print_int64_value(vpn);
		pt_base_address = pt_base_address >> 12;
		ret = pt_base_address;
	}
	printf("FUNC page_table_query RETURN : "); print_int64_value(ret);
	return ret;
}



/**
 *	The method takes 3 arguments -
 *		1. pt - physical page number of the root register in the CPU state (can assume that exists).
 *		2. vpn - virtual page number the caller wishes to map.
 *		3. ppn -
 *			if ppn is equal to NO_MAPPING value -
 *				vpn's mapping should be destroyed.
 *			else -
 *				specifies the ppn that vpn should be mapped to.
 */
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){
	printf("\nFUNC page_table_update:\n");
	printf("pt: "); print_int64_value(pt);
	printf("vpn: "); print_int64_value(vpn);
	printf("ppn: "); print_int64_value(ppn);
	
	
	uint64_t pt_base_address = (uint64_t)phys_to_virt(pt);
	uint64_t* next_pt_base_address;
	int is_valid_entry = 1;
	int level = 0;
	uint64_t index;

	for(; level < num_of_levels-1 ; level++){
		printf("level: %d.",level);
		is_valid_entry = is_valid_entry_exist(pt_base_address, level, vpn, next_pt_base_address);
		if(!is_valid_entry){ // need to create new entry
			index = get_vpn_index(vpn,level);
			printf("\n");

			printf("current pt base address:"); print_int64_value(pt_base_address); 

			*next_pt_base_address = (uint64_t)phys_to_virt(allocate_frame()); 
			printf("new_pt_address :"); print_int64_value(*next_pt_base_address); 			


			uint64_t index_pointer_address = pt_base_address + index;
			printf("pointer to next pt new address: "); print_int64_value(index_pointer_address); 
			*((uint64_t *)(index_pointer_address)) = (*next_pt_base_address) | 0x1;	
			printf("\n");
		}
		pt_base_address = *next_pt_base_address;
	}
	printf("~~~ 5_th level ~~~");
	// we have reached the 5_th level 
	ppn = ppn << 12;
	index = get_vpn_index(vpn, num_of_levels-1);
	uint64_t index_pointer_address = pt_base_address + index;
	printf("pointer to ppn address: "); print_int64_value(index_pointer_address); 
	printf("ppn : ");print_int64_value(ppn);
	*((uint64_t *)(index_pointer_address)) = ppn | 0x1;	
	printf("\nFUNC page_table_update FINISHED.\n\n\n\n");
}
/* 
			else{ // the last level - need to add the ppn
				*((uint64_t *)(pt_base_address+index)) = ppn;
			}
*/


/*
int main(){
	// uint64_t t = 12;
	int i = 0xf2;
	i = get_vpn_index(i,4);
	printf("%d\n",i);
	printf("%d\n",i << 7);
}
*/










