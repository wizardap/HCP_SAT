import subprocess
import sys
import time

if len(sys.argv) < 3:
    print("Usage: python cube_and_conquer.py <graph_path> <timeout>")
    sys.exit(1)

graph_path = sys.argv[1]
timeout = int(sys.argv[2])

# Find neighbors of vertex 1
neighbors = []
with open(graph_path, 'r') as f:
    for line in f:
        line = line.strip()
        if line.startswith('1 '):
            parts = line.split()
            if len(parts) == 2:
                neighbors.append(parts[1])

print(f"Vertex 1 has {len(neighbors)} neighbors: {neighbors}")
if len(neighbors) == 0:
    print("No neighbors found for vertex 1!")
    sys.exit(1)

processes = []
for idx, v in enumerate(neighbors):
    assume_arg = f"1,{v}"
    # Random seed to also spread the search space
    seed = idx + 1 
    cmd = [
        "./src_cpp/build/test_hcp",
        "-s", "inc_pruning_crt",
        "-d", graph_path,
        "--opt-level", "1",
        "-t", str(timeout),
        "--assume", assume_arg,
        "--seed", str(seed)
    ]
    print(f"Starting solver for partition: {assume_arg} (seed={seed})")
    
    # redirect output to files
    out_file = open(f"out_partition_{v}.log", "w")
    p = subprocess.Popen(cmd, stdout=out_file, stderr=subprocess.STDOUT)
    processes.append((p, v, out_file))

# Wait for the first SAT
start_time = time.time()
finished_v = None

while True:
    elapsed = time.time() - start_time
    if elapsed > timeout:
        print("TIMEOUT: No partition solved within the time limit.")
        break

    for p, v, out_file in processes:
        # Check log regardless of whether process exited, since it might be hanging or taking time to exit
        try:
            with open(f"out_partition_{v}.log", "r") as f:
                content = f.read()
                if "status: SAT" in content:
                    finished_v = v
                    break
        except Exception:
            pass
    
    if finished_v is not None:
        break
    time.sleep(1)

# Kill all remaining processes
for p, v, out_file in processes:
    if p.poll() is None:
        p.kill()
    out_file.close()

if finished_v is not None:
    print(f"\n--- SOLVED by partition 1,{finished_v} ---")
    with open(f"out_partition_{finished_v}.log", "r") as f:
        for line in f:
            if any(key in line for key in ["status: ", "Variables:", "Solve Time:", "check 1"]):
                print(line.strip())
else:
    print("\n--- TIMEOUT or UNSAT ---")
