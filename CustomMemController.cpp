#include <array>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <math.h>
#include <memory>

using namespace std;

#define MQ_LENGTH 10
#define MQ_PROMOTE_THRESHOLD MQ_LENGTH/2

#define MIGRATION_COST 1000 //cycles

// move the follwoing defines to input arguments to the program
#define RLDRAM_SIZE 1024*1024*1024*1 // 1GB
#define LPDRAM_SIZE 1024*1024*1024*3 // 3GB
#define CACHELINE_SIZE 64
#define NUM_CACHELINES_RLDRAM (RLDRAM_SIZE/CACHELINE_SIZE)

typedef unsigned long long addrType;

bool isAboveFilterThreshold(int currentTime, int prevTime, int queueNum);

class descriptor_T
{
    public:
    const bool inRLDRAM;
    unsigned int MQNum; // indicate which queue this descriptor is currently in
    const unsigned long long blockID; // can be the cacheline address
    unsigned long long refCounter; // reference counter
    unsigned long long prevAccessTime; // expirationTime = currentTime + lifeTime
    // unsigned long long expirationTime;

    descriptor_T(unsigned long long addr, int inRLDRAM) : 
    inRLDRAM(inRLDRAM),
    MQNum(0),
    blockID(addr>>6),
    refCounter(0),
    prevAccessTime(0) {}

    void updateAccessTime(int currentTime)
    {
        prevAccessTime = currentTime;
    }

    void updateRefCounter(int currentTime)
    {
        if(isAboveFilterThreshold(currentTime, prevAccessTime, MQNum))
        {
            refCounter++;
        }
    }
};

unordered_map<addrType, addrType> ramapTable;
unordered_map<addrType, descriptor_T *> descriptorTable;
array<queue<descriptor_T *>, MQ_LENGTH> MQ;
queue<descriptor_T *> victimQueue;

// This is used to check whether to update the refCounter
bool isAboveFilterThreshold(int currentTime, int prevTime, int queueNum)
{
    return (currentTime - prevTime) > (MIGRATION_COST/pow(2,queueNum+1));
}

void initVictimQueue()
{
    for(int addr = 0; addr < NUM_CACHELINES_RLDRAM; ++addr)
    {
        descriptor_T *ptr = new descriptor_T(addr, true); // make this a smart pointer
        // shared_ptr<descriptor_T> ptr(addr, true);
        descriptorTable[addr] = ptr;
    }
}

int main()
{
    initVictimQueue();
}
