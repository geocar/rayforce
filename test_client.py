import socket
import struct
import sys
import time

# Constants
HOST = 'localhost'
PORT = 5101
RAYFORCE_VERSION = 1  # This should match the server's version
SERDE_PREFIX = 0xcefadefa  # Magic number from server

def create_header(msg_type, size):
    """Create a message header with type and size."""
    # Match server's header_t structure:
    # u32_t prefix (0xcefadefa)
    # u8_t version (1)
    # u8_t flags (0)
    # u8_t endian (0 for little)
    # u8_t msgtype (0=async, 1=sync, 2=response)
    # u64_t size
    return struct.pack('=IBBBBBQ', 
        SERDE_PREFIX,  # prefix
        RAYFORCE_VERSION,  # version
        0,  # flags
        0,  # endian (little)
        msg_type,  # msgtype
        0,  # padding to align size
        size  # size
    )

def send_message(sock, msg_type, message):
    """Send a message with header."""
    header = create_header(msg_type, len(message))
    print(f"Sending message (type {msg_type}): {message}")
    print(f"Header bytes: {header.hex()}")
    print(f"Message bytes: {message.hex()}")
    
    # Send header and message together
    print("Sending header and message...")
    sock.sendall(header + message)

def receive_response(sock):
    """Receive and process a response."""
    print("Waiting for response...")
    try:
        # Set a longer timeout
        sock.settimeout(10.0)
        
        # First read the header (16 bytes)
        print("Reading header...")
        header_data = sock.recv(16)
        print(f"Received {len(header_data)} bytes for header")
        if len(header_data) != 16:
            print(f"Error: Invalid header received (length: {len(header_data)})")
            if len(header_data) > 0:
                print(f"Received data: {header_data.hex()}")
            return None
            
        # Unpack header
        prefix, version, flags, endian, msg_type, _, size = struct.unpack('=IBBBBBQ', header_data)
        print(f"Response prefix: 0x{prefix:08x}")
        print(f"Response version: {version}")
        print(f"Response flags: {flags}")
        print(f"Response endian: {endian}")
        print(f"Response type: {msg_type}")
        print(f"Response size: {size}")
        print(f"Header bytes: {header_data.hex()}")
        
        # Read the message body
        print(f"Reading message body ({size} bytes)...")
        data = b''
        while len(data) < size:
            chunk = sock.recv(size - len(data))
            if not chunk:
                print("Connection closed by server")
                break
            data += chunk
            print(f"Received chunk: {chunk.hex()}")
            
        return data.decode() if data else None
    except socket.timeout:
        print("Timeout waiting for response")
        return None
    except Exception as e:
        print(f"Error in receive_response: {e}")
        return None

def main():
    # Create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        # Connect to server
        print(f"Connecting to {HOST}:{PORT}...")
        sock.connect((HOST, PORT))
        
        # Perform handshake
        print("Performing handshake...")
        handshake = bytes([RAYFORCE_VERSION, 0x00])
        print(f"Handshake bytes: {handshake.hex()}")
        sock.sendall(handshake)
        
        # Wait for handshake response
        response = sock.recv(2)
        if len(response) != 2:
            print("Error: Invalid handshake response")
            return
            
        server_version = response[0]
        print(f"Server version: {server_version}")
        print(f"Handshake response bytes: {response.hex()}")
        
        # Test synchronous communication with a simple expression
        print("\nTesting synchronous communication...")
        send_message(sock, 1, b"(+ 1 2)")  # 1 = MSG_TYPE_SYNC
        
        # Give server time to process
        print("Waiting 2 seconds for server to process...")
        time.sleep(2)
        
        # Try to receive response
        response = receive_response(sock)
        if response:
            print(f"Received response: {response}")
        else:
            print("No valid response received")
            
    except Exception as e:
        print(f"Error: {e}")
    finally:
        print("Connection closed")
        sock.close()

if __name__ == "__main__":
    main() 