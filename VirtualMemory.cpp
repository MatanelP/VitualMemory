#include "VirtualMemory.h"
#include "PhysicalMemory.h"

void clearFrame (uint64_t frame)
{

  for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
      PMwrite (frame * PAGE_SIZE + i, 0);
    }
}

/*
 * Initialize the virtual memory.
 */
void VMinitialize ()
{
  clearFrame (0);
}

/*
 * extracting the offset from the given logical address
 */
uint64_t getOffset (uint64_t virtualAddress)
{
  return ((1 << OFFSET_WIDTH) - 1) & virtualAddress;
}

/*
 * extracting the physical address of the wanted page table
 * level from the given logical address (the P_i)
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

void removeReference (uint64_t parent, uint64_t frame)
{
  for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
      word_t value = 0;
      PMread (parent * PAGE_SIZE + i, &value);
      if (value == frame)
        {
          PMwrite (parent * PAGE_SIZE + i, 0);
        }
    }
}
void
updateMaxCyclical (uint64_t virtualPageNum, uint64_t page, uint64_t *currentCyclicalDistance)
{
  uint64_t d = virtualPageNum > page ?
               virtualPageNum - page : page - virtualPageNum;
  int cyclicalDistance = NUM_PAGES - d > d ? d : NUM_PAGES - d;
  *currentCyclicalDistance = cyclicalDistance > *currentCyclicalDistance ?
                             cyclicalDistance : *currentCyclicalDistance;
}
void
getFrame (uint64_t virtualPageNum, uint64_t frame, uint64_t parent,
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
              *maxFrameNum = *maxFrameNum > child ? *maxFrameNum : child;


              page = (page << OFFSET_WIDTH)
                     + row; //todo - test with a frame with more than 2 rows

              getFrame (virtualPageNum, child, frame,
                        maxFrameNum, level + 1, availableFrame,
                        evictPageFromFrameNum, parentOfEvictPageFromFrameNum,
                        currentCyclicalDistance, page, unavailableFrame, pageToEvict);

              if (*availableFrame != 0) //found availableFrame
                {
                  return;
                }
            }
        }
    }

}

word_t getAddress (uint64_t virtualAddress)
{
  uint64_t virtualPageNum = virtualAddress >> OFFSET_WIDTH;
  word_t curAddress = 0;
  word_t oldAddress = 0;
  for (int i = 0; i < TABLES_DEPTH; ++i)
    {
      oldAddress = curAddress;
      uint64_t levelOffset = getAddressForLevel (virtualAddress, i);
      PMread (oldAddress * PAGE_SIZE + levelOffset, &curAddress);
      if (curAddress == 0)
        {
          uint64_t availableFrame = 0;
          uint64_t maxFrameNum = 0;
          uint64_t evictPageFromFrameNum = 0;
          uint64_t parentOfEvictPageFromFrameNum = 0;
          uint64_t pageToEvict = 0;
          getFrame (virtualPageNum, 0, 0, &maxFrameNum,
                    0, &availableFrame, &evictPageFromFrameNum,
                    &parentOfEvictPageFromFrameNum, 0, 0, oldAddress, &pageToEvict);// get availableFrame according 3 options
          if (availableFrame == 0 || availableFrame == oldAddress)
            {
              availableFrame = maxFrameNum + 1;
            }
          else if (i < TABLES_DEPTH - 1)
            {
              clearFrame (availableFrame);
            }

          if (maxFrameNum >= NUM_FRAMES - 1)
            {
              PMevict (evictPageFromFrameNum, pageToEvict);
              removeReference (parentOfEvictPageFromFrameNum, evictPageFromFrameNum);
              availableFrame = evictPageFromFrameNum;
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
  word_t curAddress = getAddress (virtualAddress);

  PMread (curAddress * PAGE_SIZE + getOffset (virtualAddress), value);

  return 0;
}
/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite (uint64_t virtualAddress, word_t value)
{

  word_t curAddress = getAddress (virtualAddress);
  PMwrite (curAddress * PAGE_SIZE + getOffset (virtualAddress), value);

  return 0;
}
