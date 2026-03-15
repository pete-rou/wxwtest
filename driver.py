import socket
import struct
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Send some test points
for i in range(10):
    distance = 20 + (i * 5)
    angle = (i * 36) % 360
    data = struct.pack('ff', distance, angle)
    sock.sendto(data, ('127.0.0.1', 5000))
    time.sleep(0.5)

sock.close()