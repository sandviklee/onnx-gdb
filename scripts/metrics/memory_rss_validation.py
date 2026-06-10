import subprocess
import re
import statistics

if __name__ == "__main__":
    memory = []
    for i in range(30):
        result = subprocess.run(
            ["/usr/bin/time", "-l", "../../build/onnx-gdb"], capture_output=True
        )
        memory_usage = re.search(
            r"(\d+)\s+maximum resident set size", result.stderr.decode()
        )
        if not memory_usage:
            raise RuntimeError("Failed to parse memory usage from output")
        memory.append(int(memory_usage.group(1)) / (1024 * 1024))
    print(f"RSS: {memory}")
    print(f"Mean RSS: {statistics.mean(memory)} mb")
    print(f"Median RSS: {statistics.median(memory)} mb")
    print(f"Std RSS: {statistics.stdev(memory)} mb")
