import subprocess
import time
import os
import sys
import socket
import threading

def get_local_ip():
    try:
        # Create a dummy socket to connect to an external IP (doesn't actually connect)
        # to determine the interface used for routing.
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

def run_command(command, cwd=None):
    result = subprocess.run(command, shell=True, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error running command: {command}")
        print(result.stderr)
        sys.exit(1)
    return result.stdout

def stream_reader(pipe, label, output_list):
    """Reads from a pipe and appends to a list, also printing to console."""
    try:
        for line in iter(pipe.readline, ''):
            print(f"[{label}] {line.strip()}")
            output_list.append(line)
    except ValueError:
        pass

def main():
    print("--- PIGS Test Script ---")
    
    # 1. Build
    print("[1/4] Building project...")
    if not os.path.exists("build"):
        os.makedirs("build")
    
    run_command("cmake ..", cwd="build")
    run_command("make", cwd="build")
    print("Build successful.")

    # 2. Start Server
    print("[2/4] Starting Server (Port 8000, ID 1)...")
    server_process = subprocess.Popen(
        ["./server_pigs", "8000", "1"],
        cwd="build",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1  # Line buffered
    )
    
    server_output = []
    t_server = threading.Thread(target=stream_reader, args=(server_process.stdout, "SERVER", server_output))
    t_server.daemon = True
    t_server.start()

    t_server_err = threading.Thread(target=stream_reader, args=(server_process.stderr, "SERVER_ERR", server_output))
    t_server_err.daemon = True
    t_server_err.start()

    time.sleep(2) # Give it time to start and initialize

    # 3. Start Client
    print("[3/4] Starting Client (Connecting to Server Port 8000)...")
    client_process = subprocess.Popen(
        ["./client_pigs", "8000"],
        cwd="build",
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    client_output = []
    t_client = threading.Thread(target=stream_reader, args=(client_process.stdout, "CLIENT", client_output))
    t_client.daemon = True
    t_client.start()

    t_client_err = threading.Thread(target=stream_reader, args=(client_process.stderr, "CLIENT_ERR", client_output))
    t_client_err.daemon = True
    t_client_err.start()

    time.sleep(2) # Give it time to discover server

    # 4. Perform Transaction
    # The server has a hardcoded debug client "1.2.3.4" in Server.cpp
    target_ip = "1.2.3.4"
    amount = 50
    
    print(f"[4/4] Sending transaction: {amount} to {target_ip}")
    
    try:
        # Send command to client stdin
        client_process.stdin.write(f"{target_ip} {amount}\n")
        client_process.stdin.flush()
        
        # Wait a bit for processing
        time.sleep(3)
        
        print("\nStopping processes...")
        client_process.terminate()
        server_process.terminate()
        
        # Join threads (optional, but good practice if we weren't exiting)
        # t_client.join(timeout=1)
        # t_server.join(timeout=1)

        # Check results in captured output
        full_client_output = "".join(client_output)
        
        if "new_balance" in full_client_output:
            print("\nSUCCESS: Transaction confirmed by client.")
        elif "Saldo insuficiente" in full_client_output:
             print("\nSUCCESS: Transaction processed (Insufficient Balance).")
        elif "Destinatário não encontrado" in full_client_output:
             print("\nSUCCESS: Transaction processed (Recipient not found).")
        else:
            print("\nFAILURE: No transaction confirmation found in client output.")

    except Exception as e:
        print(f"An error occurred: {e}")
        client_process.kill()
        server_process.kill()

if __name__ == "__main__":
    main()
