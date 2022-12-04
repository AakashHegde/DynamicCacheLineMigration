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

#define PROMOTION_THRESHOLD 4 // Counter value at which the 

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