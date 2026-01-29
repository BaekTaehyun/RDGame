import socket
import json
import sys

# Unreal Socket Server Config
HOST = '127.0.0.1'
PORT = 3001

def send_request(command_dict):
    """
    Connects to Unreal Socket Server, sends JSON command, prints response.
    """
    print(f"--- Sending Command: {command_dict['command']} ---")
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5.0) # 5 seconds timeout
            s.connect((HOST, PORT))
            
            # Send JSON
            msg = json.dumps(command_dict)
            s.sendall(msg.encode('utf-8'))
            
            # Receive Response
            data = s.recv(1024 * 64) # 64KB buffer
            
            if not data:
                print("Error: No data received")
                return

            response = data.decode('utf-8')
            
            # Pretty print JSON response
            try:
                res_json = json.loads(response)
                print("Response:")
                print(json.dumps(res_json, indent=2))
            except json.JSONDecodeError:
                print(f"Raw Response: {response}")
                
    except ConnectionRefusedError:
        print(f"Error: Could not connect to {HOST}:{PORT}. Is the Unreal Server running?")
    except Exception as e:
        print(f"Error: {e}")
    print("-" * 30 + "\n")

if __name__ == "__main__":
    # 1. Simple Ping
    send_request({"command": "ping"})
    
    # 2. Asset Scan (Real work)
    send_request({"command": "scan_assets", "path": "/Game"})
