import socket, time, threading

HOST, PORT = "localhost", 6379
TOTAL      = 10000
THREADS    = 10
PER_THREAD = TOTAL // THREADS

def send_cmd(sock, cmd):
    sock.sendall((cmd + "\n").encode())
    return sock.recv(256).decode()

def worker(tid, results):
    s = socket.socket()
    s.connect((HOST, PORT))
    count = 0
    for i in range(PER_THREAD):
        key = f"bench:{tid}:{i}"
        send_cmd(s, f"SET {key} hello")
        send_cmd(s, f"GET {key}")
        count += 2
    s.close()
    results[tid] = count

results = [0] * THREADS
threads = []

print(f"Sending {TOTAL * 2} commands ({THREADS} threads)...")
start = time.time()

for i in range(THREADS):
    t = threading.Thread(target=worker, args=(i, results))
    threads.append(t)
    t.start()

for t in threads:
    t.join()

elapsed = time.time() - start
total_cmds = sum(results)
print(f"Done in {elapsed:.2f}s")
print(f"Total commands : {total_cmds}")
print(f"Throughput     : {int(total_cmds / elapsed):,} req/sec")
