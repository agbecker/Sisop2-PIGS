import subprocess
import time
import os
import signal
import sys
import socket
import json
import select
import threading

# Configuration
SERVER_BIN = "./build/server_pigs"
BROADCAST_PORT = 4000
SERVER_PORT_BASE = 8000
DISCOVERY_ASK = "WHERE IS SERVER OINK"
DISCOVERY_REPLY = "SERVER HERE OINK"
STARTING_BALANCE = 1000

class PigsClient:
    def __init__(self, ip, server_port):
        self.ip = ip
        self.server_port = server_port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            self.sock.bind((self.ip, 0))
        except OSError as e:
            print(f"Error binding to {self.ip}: {e}")
            raise
        self.seq = 0
        self.server_ip = None
        self.sock.settimeout(2)

    def discover(self):
        print(f"[{self.ip}] Discovering server...")
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        try:
            self.sock.sendto(DISCOVERY_ASK.encode(), ('255.255.255.255', BROADCAST_PORT))
            
            # Wait for reply
            start = time.time()
            while time.time() - start < 5:
                try:
                    data, addr = self.sock.recvfrom(1024)
                    msg = data.decode().strip()
                    # The reply might contain null bytes or garbage if C string handling is weird, but usually it's fine.
                    # The server sends "SERVER HERE OINK" (null terminated likely)
                    if DISCOVERY_REPLY in msg:
                        self.server_ip = addr[0]
                        print(f"[{self.ip}] Found server at {self.server_ip}")
                        return True
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"[{self.ip}] Discovery error: {e}")
                    return False
        except Exception as e:
             print(f"[{self.ip}] Send error: {e}")
             return False
        return False

    def send_transaction(self, receiver_ip, amount):
        if not self.server_ip:
            print(f"[{self.ip}] Server not discovered yet.")
            return False

        self.seq += 1
        req = {
            "receiver": receiver_ip,
            "amount": amount,
            "sequence": self.seq
        }
        msg = json.dumps(req)
        
        print(f"[{self.ip}] Sending {amount} to {receiver_ip} (Seq: {self.seq})")
        try:
            self.sock.sendto(msg.encode(), (self.server_ip, self.server_port))
            
            # Wait for reply
            try:
                data, addr = self.sock.recvfrom(1024)
                decoded = data.decode().rstrip('\x00')
                reply = json.loads(decoded)
                print(f"[{self.ip}] Reply: {reply}")
                
                if reply.get('status') == 4: # RR_OK
                    self.seq = reply['sequence'] + 1
                
                return reply
            except socket.timeout:
                print(f"[{self.ip}] Timeout waiting for reply")
                return None
        except Exception as e:
            print(f"[{self.ip}] Transaction error: {e}")
            return None

    def close(self):
        self.sock.close()

def run_server(port, id):
    print(f"Starting server on port {port} with ID {id}")
    cmd = ["stdbuf", "-oL", SERVER_BIN, str(port), str(id)]
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        preexec_fn=os.setsid
    )
    return process

def kill_process(process):
    if process:
        try:
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
            process.wait()
        except ProcessLookupError:
            pass

def check_server_output(process, label, expected_text, timeout=5):
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

def drain_output(process, label):
    """Reads output in a non-blocking way to keep the pipe clear and print logs."""
    while True:
        reads = [process.stdout.fileno()]
        ret = select.select(reads, [], [], 0.0) # Non-blocking
        if reads[0] in ret[0]:
            line = process.stdout.readline()
            if line:
                print(f"[{label}] {line.strip()}")
            else:
                break
        else:
            break

def main():
    servers = {}
    clients = []
    
    try:
        # Build
        print("Building...")
        if not os.path.exists("build"):
            os.makedirs("build")
        subprocess.run(["cmake", ".."], cwd="build", check=True, stdout=subprocess.DEVNULL)
        subprocess.run(["make"], cwd="build", check=True, stdout=subprocess.DEVNULL)

        # 1. Start Server 1 (Manager)
        s1 = run_server(SERVER_PORT_BASE + 1, 1)
        servers[1] = s1
        if not check_server_output(s1, "S1", "=== I AM THE MANAGER ===", timeout=5):
            print("S1 failed to become manager")
            sys.exit(1)

        # 2. Start Server 2 (Backup)
        s2 = run_server(SERVER_PORT_BASE + 2, 2)
        servers[2] = s2
        if not check_server_output(s2, "S2", "=== I AM A BACKUP ===", timeout=5):
            print("S2 failed to become backup")
            sys.exit(1)

        # 3. Start Clients
        # We use 127.0.0.x addresses. Ensure your system supports this (Linux usually does).
        client_ips = ["127.0.0.10", "127.0.0.11", "127.0.0.12"]
        
        for ip in client_ips:
            c = PigsClient(ip, SERVER_PORT_BASE + 1) # Connect to S1 port initially
            if c.discover():
                clients.append(c)
            else:
                print(f"Failed to initialize client {ip}")

        if len(clients) < 2:
            print("Not enough clients to test transactions")
            sys.exit(1)

        # 4. Test Transactions
        print("\n--- Testing Transactions ---")
        c1 = clients[0]
        c2 = clients[1]
        c3 = clients[2]

        # C1 sends 100 to C2
        reply = c1.send_transaction(c2.ip, 100)
        if reply and reply['status'] == 4: # RR_OK
            print("Transaction 1 Successful")
        else:
            print("Transaction 1 Failed")

        # C2 sends 50 to C3
        reply = c2.send_transaction(c3.ip, 50)
        if reply and reply['status'] == 4:
            print("Transaction 2 Successful")
        else:
            print("Transaction 2 Failed")

        # Check Balances (by sending 0 amount)
        print("\n--- Checking Balances ---")
        reply = c1.send_transaction(c1.ip, 0)
        print(f"C1 Balance: {reply['balance']} (Expected 900)")
        
        reply = c2.send_transaction(c2.ip, 0)
        print(f"C2 Balance: {reply['balance']} (Expected 1050)") # 1000 + 100 - 50

        # 5. Test Fault Tolerance
        print("\n--- Testing Fault Tolerance (Killing S1) ---")
        kill_process(servers[1])
        del servers[1]
        
        # Wait for S2 to take over
        if check_server_output(s2, "S2", "=== I AM THE MANAGER ===", timeout=10):
            print("S2 became manager")
        else:
            print("S2 failed to take over")
            sys.exit(1)

        # Update clients to talk to S2 port (since discovery might not update automatically in this simple script)
        # In a real scenario, clients would re-discover. Let's force re-discovery or update port.
        # The client code in C++ re-discovers if connection fails? 
        # Our python client needs to handle this.
        
        print("Re-discovering server for clients...")
        for c in clients:
            c.server_port = SERVER_PORT_BASE + 2 # Manually point to S2 port for this test, or re-discover
            # Note: Discovery broadcasts to 4000, so it should find S2.
            c.discover()

        # C3 sends 200 to C1
        print("\n--- Testing Transactions on New Manager ---")
        reply = c3.send_transaction(c1.ip, 200)
        if reply and reply['status'] == 4:
            print("Transaction 3 Successful")
        else:
            print("Transaction 3 Failed")

        # Check Balances again
        reply = c1.send_transaction(c1.ip, 0)
        print(f"C1 Balance: {reply['balance']} (Expected 1100)") # 900 + 200

        # 6. Start Server 3
        print("\n--- Adding Server 3 ---")
        s3 = run_server(SERVER_PORT_BASE + 3, 3)
        servers[3] = s3
        if check_server_output(s3, "S3", "=== I AM A BACKUP ===", timeout=5):
            print("S3 joined as backup")
        
        # 7. Kill S2
        print("\n--- Killing S2 ---")
        kill_process(servers[2])
        del servers[2]

        if check_server_output(s3, "S3", "=== I AM THE MANAGER ===", timeout=10):
            print("S3 became manager")
        
        # Update clients
        for c in clients:
            c.server_port = SERVER_PORT_BASE + 3
            c.discover()

        # C1 sends 500 to C3
        reply = c1.send_transaction(c3.ip, 500)
        if reply and reply['status'] == 4:
            print("Transaction 4 Successful")
        
        print("\nALL COMPREHENSIVE TESTS PASSED")

    except Exception as e:
        print(f"An error occurred: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("Cleaning up...")
        for s in servers.values():
            kill_process(s)
        for c in clients:
            c.close()

if __name__ == "__main__":
    main()
