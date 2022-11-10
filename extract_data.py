import re

benchmarks = ["RADIX", "FFT", "LU"]

def get_bandwidth(alldata, policy):
    for bmark, data in enumerate(alldata):
        bandwidths = re.findall("average_bandwidth\s*=?\s*(\d+.?\d+)", data)
        sum = 0     # Bytes / ns
        for i in bandwidths:
            sum += float(i)

        bw = (sum * 10**9) / (1024 ** 3) # GB/s

        print("Bandwidth (%d, %s): %.5f" % (bmark, policy, bw))

def get_latency(alldata, policy):
    for bmark, data in enumerate(alldata):
        latencies = re.findall("average_request_latency\s*=?\s*(\d+.?\d+)", data)
        sum = 0
        count = 0
        for i in latencies:
            sum += float(i)
            count += 1

        avg = sum / count

        print("Latency (%d, %s): %.0f" % (bmark, policy, avg))

def get_hits(alldata, policy):
    for bmark, data in enumerate(alldata):
        hits = re.findall(".*row_hits\s*=?\s*(\d+.?\d+)", data)
        sum = 0
        for i in hits:
            sum += int(i)

        print("Hits (%d, %s): %d" % (bmark, policy, sum))

def get_power(alldata, policy):
    for bmark, data in enumerate(alldata):
        powers = re.findall("average_power\s*=?\s*(\d+.?\d+)", data)
        cycles = float(re.findall("num_cycles\s*=?\s*(\d+.?\d+)", data)[0])

        sum = 0
        for i in powers:
            sum += float(i)

        print("Power (%d, %s): %.3f --> Efficiency (nW/cycle): %.8f" % (bmark, policy, sum, (sum / cycles) * (10**6)))

def get_idle(alldata, policy):
    for bmark, data in enumerate(alldata):
        times = re.findall("all_bank_idle_cycles.\d+\s*=?\s*(\d+.?\d+)", data)
        sum = 0
        for i in times:
            sum += int(i)

        print("Cycles (%d, %s): %d" % (bmark, policy, sum))

def main():

    closed_bmark_data = []
    for b in benchmarks:
        with open("./out_" + b + "_closed/dramsim3.txt", "r") as f:
            closed_bmark_data.append(f.read())

    open_bmark_data = []
    for b in benchmarks:
        with open("./out_" + b + "_open/dramsim3.txt", "r") as f:
            open_bmark_data.append(f.read())

    interleaved_bmark_data = []
    for b in benchmarks:
        with open("./out_" + b + "_open_interleaved/dramsim3.txt", "r") as f:
            interleaved_bmark_data.append(f.read())

    # Get Bandwidth
    # get_bandwidth(closed_bmark_data, "Closed")
    # get_bandwidth(open_bmark_data, "Open")
    get_bandwidth(interleaved_bmark_data, "Interleaved")
    print

    # Get Latency
    # get_latency(closed_bmark_data, "Closed")
    # get_latency(open_bmark_data, "Open")
    get_latency(interleaved_bmark_data, "Interleaved")
    print

    # Get Num Hits
    # get_hits(closed_bmark_data, "Closed")
    # get_hits(open_bmark_data, "Open")
    get_hits(interleaved_bmark_data, "Interleaved")
    print

    # Get Power
    get_power(closed_bmark_data, "Closed")
    get_power(open_bmark_data, "Open")
    get_power(interleaved_bmark_data, "Interleaved")
    print

    # Get Bank Idle Time
    get_idle(closed_bmark_data, "Closed")
    get_idle(open_bmark_data, "Open")
    get_idle(interleaved_bmark_data, "Interleaved")
    print

if __name__ == "__main__":
    main()
