import serial
import struct
import time
import pytest

# Target our Windows COM port
COM_PORT = 'COM8'
BAUD_RATE = 115200

@pytest.fixture(scope="module")
def pico():
    """Fixture to set up and tear down the serial connection."""
    print(f"\n[SETUP] Connecting to DUT on {COM_PORT}...")
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.5)
    time.sleep(2)  # Allow MCU to boot
    ser.reset_input_buffer()
    yield ser
    print("\n[TEARDOWN] Closing connection.")
    ser.close()

def read_pico_response(ser):
    """Helper to catch the Pico's UART output."""
    time.sleep(0.4) # Increased to 400ms to allow for massive SPI screen redraws
    if ser.in_waiting:
        return ser.readline().decode('utf-8', errors='ignore').strip()
    return ""

# ==========================================
# VALIDATION TEST CASES
# ==========================================

def test_1_valid_packet_stream(pico):
    """TEST 1: Happy Path. Inject a flawless payload."""
    pico.reset_input_buffer()
    
    magic = 0xAA
    seq = 1
    cpu = 85 # Force the core into the RED zone
    checksum = magic ^ seq ^ cpu
    
    packet = struct.pack('BBBB', magic, seq, cpu, checksum)
    pico.write(packet)
    
    response = read_pico_response(pico)
    print(f"\nDUT Output: {response}")
    
    assert "VALIDATE" in response, "Firmware rejected a valid packet!"
    assert "CPU: 85%" in response, "Firmware parsed the wrong CPU value!"

def test_2_fuzz_bad_checksum(pico):
    """TEST 2: Fault Injection. Corrupt the checksum."""
    pico.reset_input_buffer()
    
    magic = 0xAA
    seq = 2
    cpu = 50
    bad_checksum = 0x00 # Intentionally wrong math
    
    packet = struct.pack('BBBB', magic, seq, cpu, bad_checksum)
    pico.write(packet)
    
    response = read_pico_response(pico)
    print(f"\nDUT Output: {response}")
    
    assert "ERROR | CHECKSUM MISMATCH" in response, "Firmware failed to catch data corruption!"

def test_3_fuzz_fragmentation(pico):
    """TEST 3: Fault Injection. Drop the connection mid-transfer."""
    pico.reset_input_buffer()
    
    magic = 0xAA
    seq = 3
    # We purposefully pack ONLY 2 bytes, simulating a severed USB cable mid-packet
    packet = struct.pack('BB', magic, seq) 
    pico.write(packet)
    
    response = read_pico_response(pico)
    print(f"\nDUT Output: {response}")
    
    assert "ERROR | PACKET FRAGMENTED" in response, "Firmware timeout mechanism failed!"
