import serial
import time
import struct
import psutil

# Target our Windows COM port
COM_PORT = 'COM8'
BAUD_RATE = 115200
MAGIC_BYTE = 0xAA

print(f"Connecting to Reactor Core on {COM_PORT}...")
try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    time.sleep(2) # Give the serial connection a second to stabilize
    print("Connection Established! Transmitting telemetry...")
except Exception as e:
    print(f"Failed to connect on {COM_PORT}: {e}")
    print("Check Device Manager to ensure the Pico is plugged in and recognized.")
    exit()

seq_id = 0

try:
    while True:
        # 1. Sequence Tracker (0-255 loop)
        seq_id = (seq_id + 1) % 256
        
        # 2. Read CPU over a 100ms window (Smooth, accurate average)
        cpu = int(psutil.cpu_percent(interval=0.1))
        
        # 3. Calculate XOR Checksum
        # The caret (^) in Python performs a bitwise XOR operation.
        checksum = MAGIC_BYTE ^ seq_id ^ cpu
        
        # 4. Pack the new 4-byte payload: [Magic, Sequence, CPU, Checksum]
        # 'B' stands for an unsigned char (1 byte) in struct formatting
        packet = struct.pack('BBBB', MAGIC_BYTE, seq_id, cpu, checksum)
        
        # 5. Send data over USB
        ser.write(packet)
        
        # 6. Read and print the Pico's validation response
        if ser.in_waiting:
            response = ser.readline().decode('utf-8', errors='ignore').strip()
            if response:
                print(f"[PICO] {response}")
                
except KeyboardInterrupt:
    print("\nShutting down reactor telemetry...")
    ser.close()
