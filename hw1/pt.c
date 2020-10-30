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
void print_binary(uint64_t number)
{
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
 * The method gets a vpn and level end returns the index in the level's corresponding page table.
 */
uint64_t get_vpn_index(uint64_t vpn, int level){
	int offset = 7;
	return offset;
}


/**
 *	The method gets
 *		pt - page table base address
 *		pt_level - the level of the page table, integer between 0-4
 *		vpn - to search for
 *	returns 1 if entry exists, and the ppn
 *	~~~~~  add doc about the address ~~~~
 */
int is_entry_exist(uint64_t pt, int pt_level ,uint64_t vpn, uint64_t address){

	return 0;
}


/**
 * The method gets ppn and returns if is valid bit = 1
 */
int is_entry_valid(uint64_t ppn){
	return ppn & 0x1;
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
	uint64_t t = 12;
	int i = 0xf2;
	printf("%d",i);
}











