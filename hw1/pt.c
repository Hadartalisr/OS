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
 * The method gets a vpn and level (0-4),
 * returns the index in the level's corresponding page table.
 */
uint64_t get_vpn_index(uint64_t vpn, int level){
	printf("\nFUNC get_vpn_index:\n");
	printf("vpn :"); print_int64_value(vpn); print_binary(vpn);printf("\n");
	printf("level :%d\n", level);
	int offset = 7 + (level*9);
	uint64_t index = (vpn << offset) >> 55;
	/* move left that the first 9 digits will be the relevant ones
	 * and 55 to the right so at the the end the number represents the number of offset bytes */
	printf("FUNC get_vpn_index RETURN: "); print_int64_value(index);
	return index;
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
 *		pt_num - page table base NUMBER
 *		pt_level - the level of the page table, integer between 0-4
 *		vpn - to search for
 *	returns 1 if entry exists,
 *		and changes the new_pt to be the NUMBER of the new page table
 */
int is_valid_entry_exist(uint64_t pt_num, int pt_level ,uint64_t vpn, uint64_t* new_pt_num){
	printf("\nFUNC is_valid_entry_exist:\n");
	printf("pt_num: "); print_int64_value(pt_num);
	printf("pt_level: "); print_int64_value(pt_level);
	printf("vpn: "); print_int64_value(vpn);
	printf("pt_address: ");print_int64_value(phys_to_virt(pt_num));
	printf("pt_binaray_address: ");print_binary(phys_to_virt(pt_num));printf("\n"); 
	int ret = 0;
	uint64_t index = get_vpn_index(vpn, pt_level);
	uint64_t pointer_address = (uint64_t)(phys_to_virt(pt_num)) + index;
	printf("address of index in the table:"); 
	print_int64_value(pointer_address); 
	print_binary(pointer_address);printf("\n");
	uint64_t index_pointer_val = *(char*)(pointer_address);
	printf("index_pointer_val:"); 
	print_int64_value(index_pointer_val); 
	print_binary(index_pointer_val);printf("\n");
	if(is_entry_valid(index_pointer_val)){
		*new_pt_num = index_pointer_val;
		ret = 1;
	}
	else { // there is an address but it is not a valid one.
		ret = 0;
	}
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
	uint64_t* new_pt_address = (uint64_t*)calloc(1, sizeof(uint64_t*));
	uint64_t pt_base = pt;
	int is_valid_entry = 1;
	int level = 0;
	uint64_t ret = NO_MAPPING;
	for(; level < num_of_levels ; level++){
		is_valid_entry = is_valid_entry_exist(pt_base, level, vpn, new_pt_address);
		if(!is_valid_entry){
			break;
		}
		else {// need to change the parameters for the next page table level
			pt_base = *new_pt_address;
		}
	}
	if(level == 5 && is_valid_entry){ // we got to search in the 5_th level page table
		uint64_t offest = get_vpn_offset(vpn);
		ret = pt_base | offest ;
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
	
	uint64_t pt_base_num = pt;
	uint64_t new_pt_num;
	int is_valid_entry = 1;
	int level = 0;
	uint64_t index;

	for(; level < num_of_levels ; level++){
		printf("level: %d.",level);
		is_valid_entry = is_valid_entry_exist(pt_base_num, level, vpn, &new_pt_num);
		if(!is_valid_entry){ // need to create new entry
			index = get_vpn_index(vpn,level);
			if(level != num_of_levels-1){
				printf("\n");
				new_pt_num = alloc_page_frame(); 
				printf("new_pt_adress :"); 
				printf("%d",new_pt_num);
				print_int64_value(phys_to_virt(new_pt_num)); 
				print_binary(phys_to_virt(new_pt_num));printf("\n");
				
				printf("current pt base address:"); 
				printf("%d",pt_base_num);
				print_int64_value(phys_to_virt(pt_base_num)); 
				print_binary(phys_to_virt(pt_base_num));printf("\n");

				uint64_t new_pointer_address = phys_to_virt(pt_base_num)+index;
				printf("pointer to next pt new address: ");
				print_int64_value(new_pointer_address); 
				print_binary(new_pointer_address);printf("\n");
				*((char *)(new_pointer_address)) = phys_to_virt(new_pt_num);	
				printf("\n");
			}
			else{ // the last level - need to add the ppn
				*((uint64_t *)(pt_base_num+index)) = ppn;
			}
		}
		pt_base_num = (new_pt_num >> 1) << 1;
	}
}

/*
int main(){
	// uint64_t t = 12;
	int i = 0xf2;
	i = get_vpn_index(i,4);
	printf("%d\n",i);
	printf("%d\n",i << 7);
}
*/










