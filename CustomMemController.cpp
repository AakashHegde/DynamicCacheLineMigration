#include "CustomMemController.h"

static timeType currTimeStep;

class descriptor_T
{
    private:
    timeType prevAccessTime;        // expirationTime = currentTime + lifeTime
    // timeType expirationTime;
    
    public:
    const bool inRLDRAM;
    unsigned int MQNum;                 // Indicate which queue this descriptor is currently in
    const unsigned long long blockID;   // Can be the cacheline address
    unsigned long refCounter;           // Reference Counter

    descriptor_T(addrType addr, bool inRLDRAM) : 
    inRLDRAM(inRLDRAM),
    MQNum(0),
    blockID(addr >> NUM_CACHELINE_BITS),
    refCounter(0),
    prevAccessTime(0) {}

    void updateAccessTime() {
        prevAccessTime = currTimeStep;
    }

    void updateRefCounter() {
        if(isAboveFilterThreshold(prevAccessTime, MQNum))
        {
            refCounter++;
        }
    }

    friend ostream& operator<<(ostream& os, descriptor_T const& d) {
        return os << "blockID: " << d.blockID <<  " RefCounter: " << d.refCounter << " PrevAccess: " << d.prevAccessTime;
    }
};

class memoryAccess_T {
    public:
    const addrType address;
    const addrType cacheLineAddr;
    const bool isWrite;
    const timeType timeStamp;

    memoryAccess_T(addrType address, bool isWrite, timeType timeStamp) :
    address(address),
    isWrite(isWrite),
    timeStamp(timeStamp),
    cacheLineAddr(address >> NUM_CACHELINE_BITS) {}

    friend ostream& operator<<(ostream& os, memoryAccess_T const& ma) {
        return os << (ma.cacheLineAddr << NUM_CACHELINE_BITS) << "(" << (ma.address & ((1 << NUM_CACHELINE_BITS)-1))
            << ")" << " " << ma.isWrite << " " << ma.timeStamp;
    }
};

unordered_map<timeType, memoryAccess_T *> memoryAccesses;           // Array of all the Input Trace file memory accesses
unordered_map<addrType, addrType> ramapTable;                       // Hash Table to store memory access redirections
unordered_map<addrType, descriptor_T *> LPDescriptorTable;          // Hash Table to lookup descriptor for LPDRAM addresses
unordered_map<addrType, descriptor_T *> RLDescriptorTable;          // Hash Table to lookup descriptor for RLDRAM addresses
array<queue<descriptor_T *>, MQ_LENGTH> MQ;                         // Array or Queues
queue<descriptor_T *> victimQueue;                                  // Queue for the Victim Descriptors of the RLDRAM

// This is used to check whether to update the refCounter
bool isAboveFilterThreshold(int prevTime, int queueNum) {
    return (currTimeStep - prevTime) > (MIGRATION_COST / pow(2, queueNum+1));
}

bool LPDescriptorExists(addrType addr) {
    if (LPDescriptorTable.count(addr)) {
        return true;
    }
    return false;
}

descriptor_T * createLPDescriptorIfNotExists(addrType addr) {

    if (LPDescriptorExists(addr)) {
        return LPDescriptorTable[addr];
    }

    descriptor_T * d = new descriptor_T(addr, false);   //TODO: make this a smart pointer
    d->updateAccessTime();
    d->refCounter = 1;      // First memory access which is why we are creating this descriptor
    
    // Add to Descriptor Table
    LPDescriptorTable[addr] = d;

    return d;
}

void initVictimQueue()
{
    for(addrType addr = 0; addr < NUM_CACHELINES_RLDRAM; ++addr)
    {
        descriptor_T * ptr = new descriptor_T(addr, true);   //TODO: make this a smart pointer
        // shared_ptr<descriptor_T> ptr(addr, true);
        
        victimQueue.push(ptr);

        // Add to Descriptor Table
        RLDescriptorTable[addr] = ptr;
    }
}

void readTraceFile(string file, timeType * endTime) {
    ifstream trace_file;

    trace_file.open(file, ios::in);
    if (trace_file.fail()) {
        cout << "Failed to open Trace File." << endl;
        exit(1);
    }

    regex addr_exp("0[xX][0-9a-fA-F]+");
    regex type_exp("READ|WRITE");
    regex time_exp("\\d+$");
    smatch m;

    string currLine;
    addrType addr;
    bool isWrite;
    timeType time;
    bool error = false;
    while (getline(trace_file, currLine)) {
        // Get Address
        if (regex_search(currLine, m, addr_exp)) {
            for (auto x:m)
                addr = stoi(x, 0, 16);
        } else error = true;

        // Get Type
        if (regex_search(currLine, m, type_exp)) {
            for (auto x:m)
                isWrite = x == "WRITE";
        } else error = true;

        // Get Time
        if (regex_search(currLine, m, time_exp)) {
            for (auto x:m)
                time = stoi(x);
        } else error = true;

        if (error) {
            cout << "Error: Couldn't convert line." << endl;
            exit(1);
        }

        memoryAccess_T * ptr = new memoryAccess_T(addr, isWrite, time); //TODO: Make this a smart pointer
        memoryAccesses[time] = ptr;
    }

    *endTime = time;

    trace_file.close();
}

bool isMemoryAccessTimeStep(timeType t) {
    if (memoryAccesses.count(t)) return true;
    return false;
}

int main()
{
    // 1. Setup the basic Queue and Related Structures for the MQ and the Descriptors
    cout << "Initializing Victim Queue..." << endl;
    initVictimQueue();
    cout << "Victim Queue Created. Size: " << victimQueue.size() << endl;

    // 2. Read the Trace file
    cout << "Reading Input Trace File..." << endl;
    timeType traceEndTime;
    readTraceFile("traces/LU.trace", &traceEndTime);
    cout << "Completed Reading Input Trace File. Trace End Cycle: " << traceEndTime << endl;

    memoryAccess_T * currMemAccess;
    cout << "Number of LP Descriptors: " << LPDescriptorTable.size() << endl;
    cout << "Number of RL Descriptors: " << RLDescriptorTable.size() << endl;
    cout << "Victim Queue Size: " << victimQueue.size() << endl;

    // Start iteration through all time-steps until the traceEndTime
    cout << "Started Memory Controller Simulation..." << endl;
    for (currTimeStep = 0; currTimeStep <= traceEndTime; currTimeStep++) {

        if (isMemoryAccessTimeStep(currTimeStep)) {
            currMemAccess = memoryAccesses[currTimeStep];
            
            // Create new Descriptor if this is a new memory access
            descriptor_T * d = createLPDescriptorIfNotExists(currMemAccess->address);
        }

        // 3. Implement Queuing Algorithm

        // 4. Implement Remap Table

        // 5. Write to Trace File
    }

    cout << "Completed Memory Controller Simulation." << endl;

    cout << "Number of LP Descriptors: " << LPDescriptorTable.size() << endl;
    cout << "Number of RL Descriptors: " << RLDescriptorTable.size() << endl;
    cout << "Victim Queue Size: " << victimQueue.size() << endl;
}
