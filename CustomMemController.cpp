#include "CustomMemController.h"

class descriptor_T
{
    public:
    const bool inRLDRAM;
    unsigned int MQNum; // indicate which queue this descriptor is currently in
    const unsigned long long blockID; // can be the cacheline address
    unsigned long refCounter; // reference counter
    timeType prevAccessTime; // expirationTime = currentTime + lifeTime
    // timeType expirationTime;

    descriptor_T(addrType addr, int inRLDRAM) : 
    inRLDRAM(inRLDRAM),
    MQNum(0),
    blockID(addr>>NUM_CACHELINE_BITS),
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

class memoryAccess_T {
    public:
    const addrType address;
    const bool isWrite;
    const timeType timeStamp;

    memoryAccess_T(addrType address, bool isWrite, timeType timeStamp) :
    address(address),
    isWrite(isWrite),
    timeStamp(timeStamp) {}

    friend ostream& operator<<(ostream& os, memoryAccess_T const& ma) {
        return os << ma.address << " " << ma.isWrite << " " << ma.timeStamp << endl;
    }
};

unordered_map<timeType, memoryAccess_T *> memoryAccesses;
unordered_map<addrType, addrType> ramapTable;                       // Hash Table to store memory access redirections
unordered_map<addrType, descriptor_T *> descriptorTable;            // Hash Table to lookup descriptor based on incoming address
array<queue<descriptor_T *>, MQ_LENGTH> MQ;                         // Array or Queues
queue<descriptor_T *> victimQueue;                                  // Queue for the Victim Descriptors of the RLDRAM

// This is used to check whether to update the refCounter
bool isAboveFilterThreshold(int currentTime, int prevTime, int queueNum)
{
    return (currentTime - prevTime) > (MIGRATION_COST/pow(2,queueNum+1));
}

void initVictimQueue()
{
    for(int addr = 0; addr < NUM_CACHELINES_RLDRAM; ++addr)
    {
        descriptor_T *ptr = new descriptor_T(addr, true);   //TODO: make this a smart pointer
        // shared_ptr<descriptor_T> ptr(addr, true);
        descriptorTable[addr] = ptr;
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

        memoryAccess_T * ptr = new memoryAccess_T(addr, isWrite, time);
        memoryAccesses[time] = ptr;
    }

    *endTime = time;

    trace_file.close();
}

int main()
{
    // 1. Setup the basic Queue and Related Structures for the MQ and the Descriptors
    initVictimQueue();

    // 2. Read the Trace file
    timeType traceEndTime;
    readTraceFile("traces/LU.trace", &traceEndTime);

    // Print accesses
    // vector<memoryAccess_T>::iterator p;
    // for (p=memoryAccesses.begin(); p!=memoryAccesses.end(); p++) {
    //     cout << *p;
    // }

    // Start iteration through all time-steps until the traceEndTime
    for (int i = 0; i <= traceEndTime; i++) {
        // 3. Implement Queuing Algorithm

        // 4. Implement Remap Table

        // 5. Write to Trace File
    }
}
