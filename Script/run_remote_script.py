import socket
import json
import sys

# Read the script to execute
SCRIPT_TO_RUN = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\force_reload_server.py"

with open(SCRIPT_TO_RUN, "r") as f:
    UNREAL_SCRIPT = f.read()

HOST = '127.0.0.1'
PORT = 3001

def main():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10.0)
        s.connect((HOST, PORT))
        
        payload = {
            "command": "execute_python",
            "code": UNREAL_SCRIPT,
            "description": "Running Fix & Verify V10 Safe"
        }
        
        msg = json.dumps(payload)
        s.sendall(msg.encode('utf-8'))
        
        header = s.recv(4)
        if not header: return
            
        length = int.from_bytes(header, 'big')
        data = b""
        while len(data) < length:
            chunk = s.recv(min(length - len(data), 4096))
            if not chunk: break
            data += chunk
            
        print(f"Server Response:\n{json.loads(data.decode('utf-8')).get('output', '')}")
            
    except Exception as e:
        print(f"Connection Failed: {e}")
    finally:
        s.close()

if __name__ == "__main__":
    main()
