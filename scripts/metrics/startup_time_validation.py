import subprocess
import time
import statistics

if __name__ == "__main__":
    result = []
    for i in range(30):
        start = time.perf_counter()
        results = subprocess.run(["../../build/onnx-gdb"])
        end = time.perf_counter()
        ms = (end - start) * 1000
        result.append(ms)
    print(f"Execution times: {result}")
    print(f"Mean execution time: {statistics.mean(result)} ms")
    print(f"Median execution time: {statistics.median(result)} ms")
    print(f"Std: {statistics.stdev(result)} ms")
