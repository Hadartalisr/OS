#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "os.h" /* The headers which i will implement in the assignment */


#define num_of_levels 5
static uint64_t max_ppn = (2^52)-1;
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
 *	The method checks if the ppn_size is valid(if it is not more then the space of the physical memory).
 *	returns 1 if valid, else 0.
 */
int is_valid_ppn_size(uint64_t ppn){
	if (ppn <= max_ppn ){
		return 1;
	}
	return 0;
}


/**
 * The method gets a vpn and level (0-4),
 * returns the index in the level's corresponding page table.
 */
uint64_t get_vpn_index(uint64_t vpn, int level){
	int offset = 7 + (level*9);
	uint64_t index = (vpn << offset) >> 55;
	/* move left that the first 9 digits will be the relevant ones
	 * and 55 to the right so at the the end the number represents the number of offset bytes */
	return index;
}


/**
 * The method gets ppn and returns if is valid bit = 1
 */
int is_entry_valid(uint64_t ppn){
	return ppn & 0x1;
}


/**
 *	The method gets
 *		pt - page table base address
 *		pt_level - the level of the page table, integer between 0-4
 *		vpn - to search for
 *	returns 1 if entry exists, and the ppn
 *	~~~~~  add doc about the address ~~~~
 */
int is_valid_entry_exist(uint64_t pt, int pt_level ,uint64_t vpn, uint64_t* address){
	uint64_t index = get_vpn_index(vpn, pt_level);
	uint64_t new_address = *((char *)pt + index);
	if(new_address == 0){
		return 0;
	}
	else { // there is an address in the wanted index from the base of the page table.
		if(is_entry_valid(new_address)){
			*address = ((new_address >> 1) << 1) ; // need to delete to valid bit (it should be 0)
			return 1;
		}
		else { // there is an address but it is not a valid one.
			return 0;
		}
	}
}


uint64_t get_vpn_offset(uint64_t vpn){
	return ((vpn << 52) >> 52);
}


/**
 *	The method gets -
 *		pt - base address of the page table
 *		vpn - to search for
 *	returns uint64_t ppn - physical page number that the vpn is mapped to or NO_MAPPING if no mapping exists.
 */
uint64_t page_table_query(uint64_t pt, uint64_t vpn){
	uint64_t* new_pt_address = (uint64_t*)calloc(1, sizeof(uint64_t*));
	uint64_t pt_base = pt;
	int is_valid_entry = 1;
	int level = 0;
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
		return pt_base | offest ;
	}
	return NO_MAPPING;
}



/**
 *	The method takes 3 arguments -
 *		1. ppn - physical page number of the root register in the CPU state (can assume that exists).
 *		2. vpn - virtual page number the caller wishes to map.
 *		3. ppn -
 *			if ppn is equal to NO_MAPPING value -
 *				vpn's mapping should be destroyed.
 *			else -
 *				specifies the ppn that vpn should be mapped to.
 *
 */
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){
	if(!is_valid_ppn_size(ppn)){
		/* ~~~~ add code ~~~~ */
	}
}


int main(){
	// uint64_t t = 12;
	int i = 0xf2;
	i = get_vpn_index(i,4);
	printf("%d\n",i);
	printf("%d\n",i << 7);
}











