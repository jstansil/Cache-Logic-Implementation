#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/

void accessMemory(address addr, word* data, WriteEnable we)
{
  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  /* Declare variables here */
  int to_replace;
  TransferUnit transfer_size;
  unsigned int off_bits = uint_log2(block_size);
  unsigned int ind_bits = uint_log2(set_count);
  
  switch(block_size){
    case(4): transfer_size = WORD_SIZE; break;
    case(8): transfer_size = DOUBLEWORD_SIZE; break;
    case(16): transfer_size = QUADWORD_SIZE; break;
    case(32): transfer_size = OCTWORD_SIZE; break;
    default: return;
  }

  unsigned int offset = (addr) & ((1 << off_bits) - 1);
	unsigned int index = (addr >> off_bits) & ( (1 << ind_bits) - 1);
  unsigned int tag = addr >> (off_bits + ind_bits);
  
  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */

  //Increment the value tracking the age of the cache block
  /*
  if (policy == LRU){
		for (int i = 0; i < assoc; i++)
      if(cache[index].block[i].valid == VALID){
			  cache[index].block[i].lru.value++;
      }
  } */

  //Check for a cache hit by iterating through each block of the corresponding index
  for(int i = 0; i < assoc; i++){
    //HIT
    if(cache[index].block[i].valid == VALID && cache[index].block[i].tag == tag){
      //Update GUI for hits
      highlight_offset(index, i, offset, 0);
      if(policy == LRU){
        cache[index].block[i].lru.value = 1;
      }
      else{
        cache[index].block[i].accessCount++;
      }
      //READ HIT: retrieve data from cache
      if(we == READ){
        memcpy((void *)data, (void *)(cache[index].block[i].data + offset), 4);
      }
      //WRITE HIT: copy data according to memory sync policy
      else{
        //WRITE-BACK: copy data to cache block and mark it dirty to write later
        if(memory_sync_policy == WRITE_BACK){
          memcpy ((void *)(cache[index].block[i].data + offset), (void* )data, 4);
					cache[index].block[i].dirty = DIRTY;
          cache[index].block[i].valid = VALID;
        }
        //WRITE-THROUGH: copy data to cache block and then copy the entire block to memory
        else{
          memcpy ((void *)(cache[index].block[i].data + offset), (void*)data, 4);
          accessDRAM(addr - offset, (byte *)cache[index].block[i].data, transfer_size, WRITE);
					cache[index].block[i].dirty = VIRGIN;
          cache[index].block[i].valid = VALID;
        }
      } 
      return;
    }
  }
  //MISS: replace a block of the cache with the new data
  if (policy == LRU){
    //Determine the least recently used cache block (lowest lru value)
    unsigned int min = 0xFFFFFFFF;
    for (int i = 0; i < assoc; i++){
      if(min > cache[index].block[i].lru.value){
        to_replace = i;
        min = cache[index].block[i].lru.value;
      }
    }
  }
  else if (policy == LFU){
    //Determine the least frequently used cache block (lowest access count)
    unsigned int min = 0xFFFFFFFF;
    for (int i = 0; i < assoc; i++){
      if(min > cache[index].block[i].accessCount){
        to_replace = i;
        min = cache[index].block[i].accessCount;
      }
    }
  }
  else{
    //Randomly choose a cache block to replace
    to_replace = randomint(assoc);
  }
  //Update GUI for misses
  highlight_block(index, to_replace);
  highlight_offset(index, to_replace, offset, 1);

  //If write-back is enabled, check if the replaced block needs to be updated in memory
  if(memory_sync_policy == WRITE_BACK && cache[index].block[to_replace].dirty == DIRTY){
    unsigned dirtyTag = cache[index].block[to_replace].tag << (off_bits + ind_bits);
    address old_addr = dirtyTag + (index << off_bits);
    accessDRAM(old_addr, (byte *)cache[index].block[to_replace].data, transfer_size, WRITE);
  }

  //READ MISS: load the block at the given address into the corresponding cache index
  if(we == READ){
    accessDRAM(addr - offset, (byte *)cache[index].block[to_replace].data, transfer_size, READ);
    memcpy((void *) data, (void*)cache[index].block[to_replace].data, 4);
    cache[index].block[to_replace].valid = VALID;
    cache[index].block[to_replace].dirty = VIRGIN;
    cache[index].block[to_replace].tag = tag;
    if(policy == LRU){
      cache[index].block[to_replace].lru.value = 1;
    }
    else{
      cache[index].block[to_replace].accessCount = 0;
    }
  }
  //WRITE MISS:
  else{
    //WRITE ALLOCATE: load the block currently in memory into the cache, then write to that cache block
    if(memory_sync_policy == WRITE_BACK){
      accessDRAM(addr - offset, (byte *)cache[index].block[to_replace].data, transfer_size, READ);
      memcpy((void *)(cache[index].block[to_replace].data),(void * ) data, 4);
      memcpy((void *) data, (void*)cache[index].block[to_replace].data, 4);
      cache[index].block[to_replace].valid = VALID;
      cache[index].block[to_replace].dirty = DIRTY;
      cache[index].block[to_replace].tag = tag;
      if(policy == LRU){
        cache[index].block[to_replace].lru.value = 1;
      }
      else{
        cache[index].block[to_replace].accessCount = 0;
      }
    }
    //NO-WRITE ALLOCATE: write the data directly to memory, bypassing the cache
    else{
      accessDRAM(addr - offset, (byte *)cache[index].block[to_replace].data, transfer_size, WRITE);
      memcpy((void *) data, (void*)cache[index].block[to_replace].data, 4);
    }
  }
}
