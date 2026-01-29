import socket
import json
import base64
import os

HOST = '127.0.0.1'
PORT = 3001

def get_thumbnail(asset_path, save_filename="thumbnail_debug.png"):
    print(f"--- Requesting Thumbnail for: {asset_path} ---")
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(20.0) # Increased timeout for heavy assets
            s.connect((HOST, PORT))
            
            # Send Request
            msg = json.dumps({"command": "get_thumbnail", "asset_path": asset_path})
            s.sendall(msg.encode('utf-8'))
            
            # Receive Response (Length-Prefixed)
            # 1. Read 4 bytes length
            print("Waiting for header (4 bytes)...")
            header = s.recv(4)
            if not header:
                print("Error: No response from server (Connection Closed?)")
                return
                
            msg_len = int.from_bytes(header, 'big')
            print(f"Header received. Expecting {msg_len} bytes...")
            
            # 2. Read full payload
            chunks = []
            bytes_recd = 0
            while bytes_recd < msg_len:
                chunk_size = min(msg_len - bytes_recd, 1024 * 64)
                # print(f"DEBUG: asking for {chunk_size} bytes")
                chunk = s.recv(chunk_size)
                if not chunk:
                    raise RuntimeError("Socket connection broken")
                chunks.append(chunk)
                bytes_recd += len(chunk)
                # print(f"DEBUG: received {len(chunk)} (Total: {bytes_recd}/{msg_len})")
            
            print("Payload received.")
            data = b"".join(chunks)
            response = data.decode('utf-8')
            
            try:
                # Server wraps response in {"status": "success", "result": ...}
                wrapper_json = json.loads(response)
                
                if wrapper_json.get("status") == "error":
                     print(f"Error from server wrapper: {wrapper_json.get('message')}")
                     return
                     
                res_json = wrapper_json.get("result", {})
                
                if "error" in res_json:
                    print(f"Error from result: {res_json['error']}")
                    return

                if "image_data" in res_json:
                    # Decode Base64
                    b64_str = res_json["image_data"]
                    img_bytes = base64.b64decode(b64_str)
                    
                    with open(save_filename, "wb") as f:
                        f.write(img_bytes)
                    print(f"SUCCESS: Saved thumbnail to {save_filename} ({len(img_bytes)} bytes)")
                else:
                    print(f"Error: No image_data in response. Keys: {list(res_json.keys())}")

            except json.JSONDecodeError as e:
                print(f"JSON Parse Error: {e}")
                print(f"Raw len: {len(data)}")
                
    except Exception as e:
        print(f"Connection Error: {e}")
    print("-" * 30 + "\n")

if __name__ == "__main__":
    # You can change this path to an asset that exists in your project
    # Using the one found in previous step:
    target_asset = "/Game/Stylised_Dungeon_Pack/Meshes/Floors/SM_Stone_Floor_3x3" 
    
    get_thumbnail(target_asset, "thumbnail_test.png")
