import subprocess
import re
import statistics

if __name__ == "__main__":
    fps = []
    for i in range(30):
        result = subprocess.run(["../../build/onnx-gdb"], capture_output=True)
        f = re.search(r"avg FPS:\s*([\d.]+)", result.stdout.decode())
        if not f:
            raise RuntimeError("Failed to parse fps from output")
        fps.append(float(f.group(1)))
    print(f"FPS: {fps}")
    print(f"Mean FPS: {statistics.mean(fps)}")
    print(f"Median FPS: {statistics.median(fps)}")
    print(f"95 percentile: {statistics.quantiles(fps, n=100)[94]}")
    print(f"Std FPS: {statistics.stdev(fps)}")
