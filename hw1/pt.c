#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "os.h" /* The headers which i will implement in the assignment */




uint64_t page_table_query(uint64_t pt, uint64_t vpn){

	return 0;
}


/**
 * The method prints the value of uint64_t.
 */
void printIntValue(uint64_t i){
	printf("%" PRIu64 "\n", i);
	return;
}


int main(){
	uint64_t t = 1;

	printIntValue(t);
}

