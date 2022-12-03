#include "CustomMemController.h"

static timeType inputTimeStep;
static timeType outputTimeStep;

class remapEntry
{
    public:
    array<bool, NUM_CACHELINES_PER_SEGMENT> isInFastMem; // flags to indicate whether cacheline in in fast memory
    int8_t counter;

    void updateCounter(int entryIndex)
    {
        if(isInFastMem[entryIndex])
            counter = max(-128, ((int)counter - 1)); // saturate downcount at -128 (i.e. the lowest value possible for int8_t)
        else
            counter = min(127, (int)counter + 1); // saturate uncount at 127 (i.e. the highest value possible for int8_t)
    }

    bool isCounterAboveThreshold()
    {
        return counter >= PROMOTION_THRESHOLD;
    }
};

class memoryAccess_T {
    public:
    const addrType address;
    const addrType cacheLineAddr;
    const bool isWrite;
    const timeType timeStamp;
    const addrType remapIndex;  // part of cacheline address used to index to the correct remap entry
    const int entryIndex;     // part of cacheline address used to index to the correct cacheline in the entry

    memoryAccess_T(addrType address, bool isWrite, timeType timeStamp) :
    address(address),
    isWrite(isWrite),
    timeStamp(timeStamp),
    cacheLineAddr(address >> NUM_CACHELINE_BITS),
    remapIndex(cacheLineAddr & ((1 << NUM_CACHELINES_RLDRAM_BITS) - 1)),
    entryIndex(cacheLineAddr >> NUM_CACHELINES_RLDRAM_BITS)
    {}

    friend ostream& operator<<(ostream& os, memoryAccess_T const& ma) {
        // TODO: Make it print in the same format as the original trace file
        return os << ma.address << "\t" << ma.cacheLineAddr << "\t" << ma.remapIndex << "\t" << ma.entryIndex
        << "\t" << ma.isWrite << "\t" << ma.timeStamp;
    }
};

vector<memoryAccess_T *> memoryAccesses;           // Array of all the Input Trace file memory accesses
array<remapEntry, NUM_CACHELINES_RLDRAM> remapTable; // Array to store the remap entries

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
                addr = stoul(x, 0, 16);
        } else error = true;

        // Get Type
        if (regex_search(currLine, m, type_exp)) {
            for (auto x:m)
                isWrite = x == "WRITE";
        } else error = true;

        // Get Time
        if (regex_search(currLine, m, time_exp)) {
            for (auto x:m)
                time = stoul(x);
        } else error = true;

        if (error) {
            cout << "Error: Couldn't convert line." << endl;
            exit(1);
        }

        memoryAccess_T * ptr = new memoryAccess_T(addr, isWrite, time); //TODO: Make this a smart pointer
        memoryAccesses.push_back(ptr);
    }

    *endTime = time;

    trace_file.close();
}

int main()
{
    // string traceFile = "traces/LU.trace";
    // string traceFile = "traces/RADIX.trace";
    //string traceFile = "traces/FFT.trace";
    string traceFile = "traces/test.trace";

    // 2. Read the Trace file
    cout << "Reading Input Trace File..." << endl;
    timeType traceEndTime;
    readTraceFile(traceFile, &traceEndTime);
    cout << "Completed Reading Input Trace File. Trace End Cycle: " << traceEndTime << endl;

    // Start iteration through all time-steps until the traceEndTime
    cout << "Started Memory Controller Simulation..." << endl;
    cout << "---------------------------------------" << endl;

    memoryAccess_T * currMemAccess;

    // Iterate through all the lines read from the trace file
    for(int inputTraceIdx = 0; inputTraceIdx < memoryAccesses.size(); ++inputTraceIdx)
    {
        currMemAccess = memoryAccesses[inputTraceIdx];

        // Update output time step to be used in output trace file time stamp
        outputTimeStep = max(outputTimeStep, inputTimeStep);

        remapEntry currentEntry = remapTable[currMemAccess->remapIndex];
        currentEntry.updateCounter(currMemAccess->entryIndex);

        if(currentEntry.isCounterAboveThreshold())
        {
            //TODO migrate
        }
        else
        {
            //forward address to trace file
        }


        // 4. Implement Remap Table

        // 5. Write to Trace File
            // TODO: When we write to the RLDRAM trace file, we need to OR the cacheline bits (lower 6 bits) of the incomming
            //       address with the blockID/cacheline base address for that remapped address.
            //       In the case of writing to the LPDRAM trace file, just write the incoming address as it is.
    }


    cout << "---------------------------------------" << endl;
    cout << "Completed Memory Controller Simulation." << endl;
}
