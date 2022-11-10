#include <array>
#include <unordered_map>
#include <queue>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>
#include <memory>
#include <string>
#include <regex>

using namespace std;

#define MQ_LENGTH 10
#define MQ_PROMOTE_THRESHOLD MQ_LENGTH/2

#define MIGRATION_COST 1000 //cycles

// move the follwoing defines to input arguments to the program
#define RLDRAM_SIZE 1024*1024*1024*1 // 1GB
#define LPDRAM_SIZE 1024*1024*1024*3 // 3GB
#define CACHELINE_SIZE 64
#define NUM_CACHELINE_BITS int(log2(CACHELINE_SIZE))
#define NUM_CACHELINES_RLDRAM (RLDRAM_SIZE/CACHELINE_SIZE)

typedef unsigned long long addrType;
typedef unsigned long long timeType;

bool isAboveFilterThreshold(int currentTime, int prevTime, int queueNum);