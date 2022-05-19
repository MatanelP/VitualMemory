#include "VirtualMemory.h"
#include "PhysicalMemory.h"

/*
 * Initialize the virtual memory.
 */
void VMinitialize(){
  //  clearing frame 0:
  for(uint64_t i = 0 ; i < PAGE_SIZE ; i++)
    PMwrite (i, 0);
}

/*
 * extracting the offset from the given logical address
 */
uint64_t getOffset(uint64_t virtualAddress){
  return ((1 << OFFSET_WIDTH) - 1) & virtualAddress;
}

/*
 * extracting the physical address of the wanted page table
 * level from the given logical address (the P_i)
 */
uint64_t getAddressForLevel(uint64_t virtualAddress, int level){
    // assert(level < TABLES_DEPTH)
    if (level == 0){
        return virtualAddress >> ((TABLES_DEPTH) * OFFSET_WIDTH);
    }
    return virtualAddress >> ((TABLES_DEPTH - level) * OFFSET_WIDTH) & ((1 << OFFSET_WIDTH) - 1);
}


/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value){
  uint64_t offset = getOffset (virtualAddress);


  return 0;
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value){

  return 0;
}
