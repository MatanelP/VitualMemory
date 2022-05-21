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


uint64_t getFrame(uint64_t frame, uint64_t parent, uint64_t maxFrame){
    if (frame != 0){
        // 1. check for empty table:

        if(isEmptyFrame(frame)){ //all rows are 0's
        //  remove reference from parent
        removeReference(parent, frame);
        return frame;
        }

        //2. unused frame:
        //

        //3. minimum cyclical distance:

    }

    for (uint64_t child = 0 ; child < PAGE_SIZE ; child++){
        maxFrame = maxFrame > child ? maxFrame : child;
        getFrame(child, frame, maxFrame);
    }



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

  uint64_t curAddress = 0;
  uint64_t oldAddress = 0;
  for (int i =0; i < TABLES_DEPTH; ++i){
      //oldAddress = curAddress;
      curAddress = getAddressForLevel(virtualAddress, i);
      PMread(oldAddress * PAGESIZE + curAddress, (word_t*) curAddress);
      if (curAddress == 0){
          uint64_t frame = getFrame(0, 0, 0);// get frame according 3 options
          if (i < TABLES_DEPTH - 1){
              // only need to clear frame if next layer is a table
              clearFrame(frame);
          }
          else{
              // next layer is not a table, restore page we are looking for into frame
              //PMrestore(frame, restoredPageIndex);  curAddress?
          }
          // point to new page table from its parent
          PMwrite(oldAddress * PAGESIZE + curAddress, (word_t) frame);
          oldAddress = curAddress;
      }
  }
  PMread(oldAddress * PAGESIZE + offset, value);

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
