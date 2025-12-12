import subprocess
import time
import os
import signal
import sys

import select

def run_server(port, id):
    print(f"Starting server on port {port} with ID {id}")
    # Use stdbuf to disable buffering so we see output immediately
    cmd = ["stdbuf", "-oL", "./build/server_pigs", str(port), str(id)]
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        preexec_fn=os.setsid # To allow killing the whole process group
    )
    return process

def check_output(process, label, expected_text, timeout=5):
    start_time = time.time()
    while time.time() - start_time < timeout:
        reads = [process.stdout.fileno()]
        ret = select.select(reads, [], [], 0.1)

        if reads[0] in ret[0]:
            line = process.stdout.readline()
            if line:
                print(f"[{label}] {line.strip()}")
                if expected_text in line:
                    return True
    return False

def kill_process(process):
    if process:
        try:
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
            process.wait()
        except ProcessLookupError:
            pass

def main():
    s1 = None
    s2 = None
    s3 = None

    try:
        # Build
        print("Building...")
        subprocess.run(["cmake", ".."], cwd="build", check=True, stdout=subprocess.DEVNULL)
        subprocess.run(["make"], cwd="build", check=True, stdout=subprocess.DEVNULL)

        # 1. Start Server 1 (ID 1)
        s1 = run_server(8001, 1)
        
        # Expect S1 to become manager
        if check_output(s1, "S1", "=== I AM THE MANAGER ===", timeout=5):
            print("SUCCESS: S1 became manager")
        else:
            print("FAILURE: S1 did not become manager")
            sys.exit(1)

        # 2. Start Server 2 (ID 2)
        s2 = run_server(8002, 2)

        # Expect S2 to become backup (since S1 is running)
        if check_output(s2, "S2", "=== I AM A BACKUP ===", timeout=5):
            print("SUCCESS: S2 became backup")
        else:
            print("FAILURE: S2 did not become backup")
            sys.exit(1)

        # 3. Kill S1
        print("Killing S1...")
        kill_process(s1)
        s1 = None

        # Expect S2 to become manager
        if check_output(s2, "S2", "=== I AM THE MANAGER ===", timeout=10):
            print("SUCCESS: S2 became manager after S1 death")
        else:
            print("FAILURE: S2 did not take over")
            sys.exit(1)

        # 4. Start Server 3 (ID 3)
        s3 = run_server(8003, 3)

        # Expect S3 to become backup
        if check_output(s3, "S3", "=== I AM A BACKUP ===", timeout=5):
            print("SUCCESS: S3 became backup")
        else:
            print("FAILURE: S3 did not become backup")
            sys.exit(1)

        # 5. Kill S2
        print("Killing S2...")
        kill_process(s2)
        s2 = None

        # Expect S3 to become manager
        if check_output(s3, "S3", "=== I AM THE MANAGER ===", timeout=10):
            print("SUCCESS: S3 became manager after S2 death")
        else:
            print("FAILURE: S3 did not take over")
            sys.exit(1)

        print("\nALL TESTS PASSED")

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        print("Cleaning up...")
        kill_process(s1)
        kill_process(s2)
        kill_process(s3)

if __name__ == "__main__":
    main()
