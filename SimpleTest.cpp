#include <cassert>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "stdio.h"
#include "MemoryConstants.h"

void print_physical ()
{
  word_t val;
  for (int i = 0; i < NUM_FRAMES; i++)
    {
      for (int row = 0; row < PAGE_SIZE; row++)
        {
          PMread (i * PAGE_SIZE + row, &val);
          printf ("frame: %d | row: %d | val: %d\n", i, row, val);
        }
    }
}

int main (int argc, char **argv)
{
  VMinitialize ();
  for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i)
    {
      printf ("writing to %llu\n", (long long int) i);
      VMwrite (5 * i * PAGE_SIZE, i);
    }

  for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i)
    {
      word_t value;
      VMread (5 * i * PAGE_SIZE, &value);
      printf ("reading from %llu %d\n", (long long int) i, value);
      assert(uint64_t(value) == i);
    }
  printf ("success\n");


//  printf("---- VMinitialize ----\n");
//  VMinitialize();
//  print_physical();
//
//  printf("---- VMwrite(13, 3) ----\n");
//  printf("virtual add: 13 (01101), page: 6 (0110), offset: 1 (1) \n");
//  VMwrite(13, 3);
//  word_t val;
//  VMread(13, &val);
//  print_physical(); // compare to slide 14
//
//  printf("---- VMread(6, &val) ----\n");
//  printf("virtual add: 6 (00110), page: 3 (0011), offset: 0 (0) \n");
//  VMread(6, &val);
//  print_physical(); // compare to slide 16
//
//  printf("---- VMread(31, &val);----\n");
//  printf("virtual add: 31 (11111), page: 15 (1111), offset: 1 (1) \n");
//  VMread(31, &val);
//  print_physical(); // compare to slide 27, notice that second row in frame 2 should be 7


  return 0;
}
