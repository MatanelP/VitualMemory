#include "VirtualMemory.h"
#include "PhysicalMemory.h"

/**
 * Clearing a given frame
 * @param frame
 */
void clearFrame (uint64_t frame)
{
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
        PMwrite (frame * PAGE_SIZE + i, 0);
    }
}

/**
 * Initializing the virtual memory
 */
void VMinitialize ()
{
    clearFrame (0);
}

/**
 * Extracting the offset from the given logical address
 * @param virtualAddress
 * @return
 */
uint64_t getOffset (uint64_t virtualAddress)
{
    return ((1 << OFFSET_WIDTH) - 1) & virtualAddress;
}

/**
 * Extracting the physical address of the wanted page table
 * Level from the given logical address (the P_i)
 * @param virtualAddress
 * @param level
 * @return physical address
 */
uint64_t getAddressForLevel (uint64_t virtualAddress, int level)
{
    // assert(level < TABLES_DEPTH)
    if (level == 0)
    {
        return virtualAddress >> ((TABLES_DEPTH) * OFFSET_WIDTH);
    }
    return virtualAddress >> ((TABLES_DEPTH - level) * OFFSET_WIDTH)
           & ((1 << OFFSET_WIDTH) - 1);
}

bool isEmptyFrame (uint64_t frame)
{
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
        word_t value = 0;
        PMread (frame * PAGE_SIZE + i, &value);
        if (value != 0) return false;
    }
    return true;
}

/**
 * Helper function for case 1 of runDFS
 * Removing references to this table from its parent
 * @param parent
 * @param frame
 */
void removeReference (uint64_t parent, uint64_t frame)
{
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
        word_t value = 0;
        PMread (parent * PAGE_SIZE + i, &value);
        if (value == (word_t) frame)
        {
            PMwrite (parent * PAGE_SIZE + i, 0);
        }
    }
}

/**
 * Helper function updating the cyclical distance, the max/min formula used in case 3 of runDFS
 * @param virtualPageNum
 * @param page
 * @param currentCyclicalDistance
 */
void
updateMaxCyclical (uint64_t virtualPageNum, uint64_t page, uint64_t *currentCyclicalDistance)
{
    uint64_t d = virtualPageNum > page ?
                 virtualPageNum - page : page - virtualPageNum;
    uint64_t cyclicalDistance = NUM_PAGES - d > d ? d : NUM_PAGES - d;
    *currentCyclicalDistance = cyclicalDistance > *currentCyclicalDistance ?
                               cyclicalDistance : *currentCyclicalDistance;
}

/**
 * Helper DFS function to retrieve a frame. A frame is chosen from a priority list, from best case to worst case:
 * 1) A frame containing an empty table
 * 2) An unused frame
 * 3) Evicting some page from a frame, chosen using a max/min formula
 * @param virtualPageNum
 * @param frame
 * @param parent
 * @param maxFrameNum
 * @param level
 * @param availableFrame
 * @param evictPageFromFrameNum
 * @param parentOfEvictPageFromFrameNum
 * @param currentCyclicalDistance
 * @param page
 * @param unavailableFrame
 * @param pageToEvict
 */
void
runDFS (uint64_t virtualPageNum, uint64_t frame, uint64_t parent,
        uint64_t *maxFrameNum, int level, uint64_t *availableFrame,
        uint64_t *evictPageFromFrameNum, uint64_t *parentOfEvictPageFromFrameNum,
        uint64_t currentCyclicalDistance, uint64_t page, uint64_t unavailableFrame, uint64_t *pageToEvict)
{
    if (frame != 0)
    {
        //3. calc minimum cyclical distance and update maximum:
        if (level == TABLES_DEPTH)
        {
            uint64_t currentlyDist = currentCyclicalDistance;
            updateMaxCyclical (virtualPageNum, page, &currentCyclicalDistance);
            if (currentlyDist != currentCyclicalDistance)
            {
                *evictPageFromFrameNum = frame;
                *parentOfEvictPageFromFrameNum = parent;
            }
            *pageToEvict = page;
        }


        // 1. check for empty table:
        if (isEmptyFrame (frame) && level != TABLES_DEPTH
            && frame != unavailableFrame)
        { //all rows are 0's
            //  remove reference from parent
            removeReference (parent, frame);
            *availableFrame = frame;
            *maxFrameNum = 0;
            return;
        }

    }

    // going through the current frame's children and recurse if needed
    for (uint64_t row = 0; row < PAGE_SIZE; row++)
    {
        word_t child = 0;
        if (level != TABLES_DEPTH)
        {
            PMread (frame * PAGE_SIZE + row, &child);

            if (child != 0)
            {
                *maxFrameNum = *maxFrameNum > (uint64_t) child ? *maxFrameNum : (uint64_t) child;

                runDFS(virtualPageNum, child, frame,
                       maxFrameNum, level + 1, availableFrame,
                       evictPageFromFrameNum, parentOfEvictPageFromFrameNum,
                       currentCyclicalDistance, (page << OFFSET_WIDTH)
                                                + row, unavailableFrame, pageToEvict);

                if (*availableFrame != 0) //found availableFrame
                {
                    return;
                }
            }
        }
    }

}

uint64_t getFrame(uint64_t virtualPageNum, word_t oldAddress){

    uint64_t maxFrameNum = 0;
    uint64_t availableFrame = 0;
    uint64_t evictPageFromFrameNum = 0;
    uint64_t parentOfEvictPageFromFrameNum = 0;
    uint64_t pageToEvict = 0;
    runDFS(virtualPageNum, 0, 0, &maxFrameNum,
           0, &availableFrame, &evictPageFromFrameNum,
           &parentOfEvictPageFromFrameNum, 0, 0,
           oldAddress, &pageToEvict);
    if (availableFrame == 0 || availableFrame == (uint64_t) oldAddress)
    {
        availableFrame = maxFrameNum + 1;
    }


    if (maxFrameNum >= NUM_FRAMES - 1)
    {
        PMevict (evictPageFromFrameNum, pageToEvict);
        removeReference (parentOfEvictPageFromFrameNum, evictPageFromFrameNum);
        availableFrame = evictPageFromFrameNum;
    }

    return availableFrame;


}

/**
 * Retrieving an address for a given virtual address
 * @param virtualAddress
 * @return Physical address to read/write from
 */
word_t getAddress (uint64_t virtualAddress)
{
    uint64_t virtualPageNum = virtualAddress >> OFFSET_WIDTH;
    word_t curAddress = 0;
    word_t oldAddress = 0;
    uint64_t availableFrame;
    for (int i = 0; i < TABLES_DEPTH; ++i)
    {
        oldAddress = curAddress;
        uint64_t levelOffset = getAddressForLevel (virtualAddress, i);
        PMread (oldAddress * PAGE_SIZE + levelOffset, &curAddress);
        if (curAddress == 0)
        {
            availableFrame = getFrame(virtualPageNum, oldAddress);

            if (i < TABLES_DEPTH - 1)
            {// next layer is a table, need to clear contents of frame
                clearFrame (availableFrame);
            }

            // point to new page table from its parent
            PMwrite (oldAddress * PAGE_SIZE + levelOffset, availableFrame);
            curAddress = availableFrame;
        }
    }

    PMrestore (curAddress, virtualPageNum);
    return curAddress;
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread (uint64_t virtualAddress, word_t *value)
{
    if (VIRTUAL_MEMORY_SIZE <= virtualAddress) {
        return 0;
    }

    word_t curAddress = getAddress (virtualAddress);

    PMread (curAddress * PAGE_SIZE + getOffset (virtualAddress), value);

    return 1;
}
/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite (uint64_t virtualAddress, word_t value)
{

    if (VIRTUAL_MEMORY_SIZE <= virtualAddress) {
        return 0;
    }

    word_t curAddress = getAddress (virtualAddress);
    PMwrite (curAddress * PAGE_SIZE + getOffset (virtualAddress), value);

    return 1;
}