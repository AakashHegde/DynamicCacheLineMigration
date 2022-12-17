#ifndef CUSTOMMEMCONTROLLER_H
#define CUSTOMMEMCONTROLLER_H

#include <array>
#include <unordered_map>
#include <queue>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <math.h>
#include <memory>
#include <string>
#include <regex>

using namespace std;

#define WRITE true
#define READ !WRITE

#define MIGRATION_COST 1000 // Cycles

#define PROMOTION_THRESHOLD 16 // Counter value at which the 

// move the follwoing defines to input arguments to the program
#define RLDRAM_SIZE (1024U*1024U*1024U*1U) // 1GB
#define LPDRAM_SIZE (1024U*1024U*1024U*4UL) // 4GB
#define CACHELINE_SIZE 64
#define NUM_CACHELINE_BITS int(log2(CACHELINE_SIZE))
#define NUM_CACHELINES_RLDRAM (RLDRAM_SIZE/CACHELINE_SIZE)
#define NUM_CACHELINES_RLDRAM_BITS int(log2(NUM_CACHELINES_RLDRAM)) 

#define NUM_CACHELINES_PER_SEGMENT (LPDRAM_SIZE/RLDRAM_SIZE)

typedef unsigned int addrType;
typedef unsigned long long timeType;

void writeToTraceFile(ofstream &file, addrType address, bool isWrite);
addrType translateAddress(int remapIndex);

// class remapEntry: Store information about one entry in the remap table
class remapEntry
{
    public:
    
    // isInFastMem: flags to indicate which segment/cache line is in fast memory
    array<bool, NUM_CACHELINES_PER_SEGMENT> isInFastMem;
    // counter: shared counter for all segments in this entry
    int8_t counter;

    // remapEntry(): initialize flags and counter to 0
    remapEntry();

    // updateCounter(): update the shared counter for this entry
    void updateCounter(int entryIndex);

    // resetCounter(): reset the shared counter for this entry
    void resetCounter();

    // resetFlags(): reset the flags for this entry
    void resetFlags();

    // findTrueFlag(): find which segment in this entry is present in fast memory; return -1 if no segment is in fast memory
    int findTrueFlag();

    // isCounterAboveThreshold(): return true if the shared counter is above the migration threshold
    bool isCounterAboveThreshold();

    // isEntryEmpty(): return true if none of the segments in this entry are in fast memory
    bool isEntryEmpty();
};

// class memoryAccess: extract and store required information from the address read from the input trace file
class memoryAccess 
{
    public:

    // address: the address as read from the input trace file
    const addrType address;
    // cacheLineAddr: cache line address extracted form the full address
    const addrType cacheLineAddr;
    // isWrite: flag set to true if the operation is a write; set to false if read operation
    const bool isWrite;
    // timeStamp: time stamp at which this operation was issued
    const timeType timeStamp;
    // remapIndex: part of cache line address used to index to the correct remap entry
    const addrType remapIndex;
    // entryIndex: part of cache line address used to index to the correct segment in the entry
    const int entryIndex;

    memoryAccess(addrType address, bool isWrite, timeType timeStamp);

    // overload the outstream operator to conviniently print out relevant information from objects of this class
    friend ostream& operator<<(ostream& os, memoryAccess const& ma);
};

#endif // CUSTOMMEMCONTROLLER_H
