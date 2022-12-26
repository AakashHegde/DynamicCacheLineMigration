#include "CustomMemController.h"

static timeType inputTimeStep;
static timeType outputTimeStep;

// Statistics
static int numSwaps;
static int numMoveToFast;
static int numMoveToSlow;

class remapEntry
{
    public:
    // TODO: instead of storing a TRUE/FALSE array, can't we just store the encoded binary value of which cacheline is stored here?
    //       - Change the updateCounter function: IF condition becomes: if (entryIndex == currLineInFastMem)


    array<bool, NUM_CACHELINES_PER_SEGMENT> isInFastMem; // flags to indicate whether cacheline in in fast memory
    int8_t counter;

    remapEntry() {
        for (int i = 0; i < NUM_CACHELINES_PER_SEGMENT; i++) {
            isInFastMem[i] = false;
        }
        counter = 0;
    }

    void updateCounter(int entryIndex)
    {
        if(isInFastMem[entryIndex])
            // TODO: experiment with using negative count or not
            // counter = max(-128, ((int)counter - 1)); // saturate downcount at -128 (i.e. the lowest value possible for int8_t)
            counter = max(0, ((int)counter - 1));
        else
            counter = min(127, (int)counter + 1); // saturate uncount at 127 (i.e. the highest value possible for int8_t)
    }

    void resetCounter() {
        counter = 0;
    }

    void resetFlags() {
        for (auto i: isInFastMem) {
            i = false;
        }
    }

    int findTrueFlag() {
        for (int i = 0; i < NUM_CACHELINES_PER_SEGMENT; i++) {
            if (isInFastMem[i]) return i;
        }
        return -1;
    }

    bool isCounterAboveThreshold()
    {
        return counter >= PROMOTION_THRESHOLD;
    }
};

class memoryAccess {
    public:
    const addrType address;
    const addrType cacheLineAddr;
    const bool isWrite;
    const timeType timeStamp;
    const addrType remapIndex;  // part of cacheline address used to index to the correct remap entry
    const int entryIndex;     // part of cacheline address used to index to the correct cacheline in the entry

    memoryAccess(addrType address, bool isWrite, timeType timeStamp) :
    address(address),
    isWrite(isWrite),
    timeStamp(timeStamp),
    cacheLineAddr(address >> NUM_CACHELINE_BITS),
    remapIndex(cacheLineAddr & ((1 << NUM_CACHELINES_RLDRAM_BITS) - 1)),
    entryIndex(cacheLineAddr >> NUM_CACHELINES_RLDRAM_BITS)
    {}

    friend ostream& operator<<(ostream& os, memoryAccess const& ma) {
        return os << ma.address << "\t" << ma.cacheLineAddr << "\t" << ma.remapIndex << "\t" << ma.entryIndex
        << "\t" << ma.isWrite << "\t" << ma.timeStamp;
    }
};

vector<memoryAccess *> memoryAccesses;                // Array of all the Input Trace file memory accesses
array<remapEntry, NUM_CACHELINES_RLDRAM> remapTable;    // Array to store the remap entries

void printRemapTable(int remapIndex) {
    remapEntry re = remapTable[remapIndex];
    printf("RemapEntry: %d, Counter=%d, entryIndices=", remapIndex, re.counter);
    for (int i = 0; i < NUM_CACHELINES_PER_SEGMENT; i++) {
        printf("%d", re.isInFastMem[i]);
    }
    printf("\n");
}

int countNumFastMemCacheLines() {
    int count = 0;
    for (int i = 0; i < remapTable.size(); i++) {
        for (auto j : remapTable[i].isInFastMem) {
            if (j) {
                count++;
                break;
            }
        }
    }
    return count;
}

void readTraceFile(string file, timeType * endTime) {
    ifstream trace_file;

    trace_file.open(file, ios::in);
    if (trace_file.fail()) {
        cout << "Failed to open Trace File: " << file << endl;
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

        memoryAccess * ptr = new memoryAccess(addr, isWrite, time); //TODO: Make this a smart pointer
        memoryAccesses.push_back(ptr);
    }

    *endTime = time;

    trace_file.close();
}

bool isEntryEmpty(array<bool, NUM_CACHELINES_PER_SEGMENT>& isInFastMem) {
    for (auto i: isInFastMem) {
        if (i) return false;
    }
    return true;
}

void migrateCacheline(remapEntry * currentEntry, memoryAccess * currMemAccess,
                      ofstream& RLTraceFileStream, ofstream& LPTraceFileStream) {

    // Check if this entry has no cachelines in fast memory
    if (isEntryEmpty(currentEntry->isInFastMem)) {
        if (!currMemAccess->isWrite) {
            writeToTraceFile(LPTraceFileStream, currMemAccess->address, READ);
            outputTimeStep++;
        }

        writeToTraceFile(RLTraceFileStream, translateAddress(currMemAccess->remapIndex), WRITE);
        outputTimeStep++;
    } else {
        // Swapping occurs here; number of steps for swap depends on READ or WRITE
        int previousEntryIndex = currentEntry->findTrueFlag();
        addrType previousAddress = previousEntryIndex << (NUM_CACHELINE_BITS + NUM_CACHELINES_RLDRAM_BITS);
        previousAddress |= currMemAccess->remapIndex << NUM_CACHELINE_BITS;

        if (currMemAccess->isWrite) {
            writeToTraceFile(RLTraceFileStream, translateAddress(currMemAccess->remapIndex), READ);
            outputTimeStep++;
        } else {
            writeToTraceFile(LPTraceFileStream, currMemAccess->address, READ);
            writeToTraceFile(RLTraceFileStream, translateAddress(currMemAccess->remapIndex), READ);
            outputTimeStep++;
        }
        writeToTraceFile(LPTraceFileStream, previousAddress, WRITE);
        writeToTraceFile(RLTraceFileStream, translateAddress(currMemAccess->remapIndex), WRITE);
        outputTimeStep++;
    }

    // Update Remap Entry
    currentEntry->resetFlags();
    currentEntry->isInFastMem[currMemAccess->entryIndex] = true;
    currentEntry->resetCounter();
}
void writeToTraceFile(ofstream &file, addrType address, bool isWrite) {
    file << "0x";
    file.width(8);
    file.fill('0');
    file << hex << uppercase << address << " ";
    if (isWrite) {
        file << "WRITE" << " ";
    } else {
        file << "READ" << " ";
    }
    file << dec << outputTimeStep << endl;
}

addrType translateAddress(int remapIndex) {
    addrType newAddress;

    newAddress = remapIndex << NUM_CACHELINE_BITS;
    // newAddress |= oldAddress & ((1 << NUM_CACHELINE_BITS) - 1);

    return newAddress;
}

int main()
{
    string inputTraceFileName = "LU";
    // string inputTraceFileName = "FFT";
    // string inputTraceFileName = "RADIX";
    // string inputTraceFileName = "testEntryIndexing";
    // string inputTraceFileName = "testMigrations";

    string traceFile   = "traces/" + inputTraceFileName + ".trace";
    string RLTraceFile = "traces/" + inputTraceFileName + "_RL" + ".trace";
    string LPTraceFile = "traces/" + inputTraceFileName + "_LP" + ".trace";

    ofstream RLTraceFileStream, LPTraceFileStream;

    RLTraceFileStream.open(RLTraceFile, ios::out);
    if (RLTraceFileStream.fail()) {
        cout << "Failed to open RL Trace File." << endl;
        exit(1);
    }
    LPTraceFileStream.open(LPTraceFile, ios::out);
    if (LPTraceFileStream.fail()) {
        cout << "Failed to open LP Trace File." << endl;
        exit(1);
    }

    numSwaps = 0;
    numMoveToFast = 0;
    numMoveToSlow = 0;

    outputTimeStep = 0;

    // 2. Read the Trace file
    cout << "Reading Input Trace File..." << endl;
    timeType traceEndTime;
    readTraceFile(traceFile, &traceEndTime);
    cout << "Completed Reading Input Trace File. Trace End Cycle: " << traceEndTime << endl;

    // Start iteration through all time-steps until the traceEndTime
    cout << "Started Memory Controller Simulation..." << endl;
    cout << "---------------------------------------" << endl;

    memoryAccess * currMemAccess;
    remapEntry * currentEntry;
    int numMigrations = 0;

    // Iterate through all the lines read from the trace file
    for(int inputTraceIdx = 0; inputTraceIdx < memoryAccesses.size(); ++inputTraceIdx)
    {
        currMemAccess = memoryAccesses[inputTraceIdx];
        
        inputTimeStep = currMemAccess->timeStamp;

        // Update output time step to be used in output trace file time stamp
        outputTimeStep = max(outputTimeStep, inputTimeStep);

        currentEntry = &remapTable[currMemAccess->remapIndex];

        currentEntry->updateCounter(currMemAccess->entryIndex);

        // printRemapTable(currMemAccess->remapIndex);
        if(currentEntry->isCounterAboveThreshold())
        {
            // cout << "---------- Migration Performed ----------" << endl;
            migrateCacheline(currentEntry, currMemAccess, RLTraceFileStream, LPTraceFileStream);
            printf("Address: 0x%x, RemapIndex: %d, EntryIndex: %d\n",
                    currMemAccess->address, currMemAccess->remapIndex, currMemAccess->entryIndex);
            numMigrations++;
        }
        else
        {
            if (currentEntry->isInFastMem[currMemAccess->entryIndex]) {
                // Write to RL Tracefile with RemapIndex as the address
                writeToTraceFile(RLTraceFileStream, translateAddress(currMemAccess->remapIndex), currMemAccess->isWrite);
            } else {
                // Write to LP Tracefile with original address
                writeToTraceFile(LPTraceFileStream, currMemAccess->address, currMemAccess->isWrite);
            }
            outputTimeStep++;
        }
    }

    RLTraceFileStream.close();
    LPTraceFileStream.close();

    cout << "---------------------------------------" << endl;
    cout << "Completed Memory Controller Simulation." << endl;
    cout << "---------------------------------------" << endl;
    cout << "Number of Migrations: " << numMigrations << endl;
    cout << "Remap Table Size: " << countNumFastMemCacheLines() << endl;
}
