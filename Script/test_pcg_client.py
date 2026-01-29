import socket
import json
import time

HOST = '127.0.0.1'
PORT = 3001

def send_request(sock, command_data):
    """Sends a request and returns the parsed JSON result."""
    msg = json.dumps(command_data)
    sock.sendall(msg.encode('utf-8'))
    
    # Read Header
    header = sock.recv(4)
    if not header:
        print("Error: No header received")
        return None
        
    msg_len = int.from_bytes(header, 'big')
    
    # Read Payload
    chunks = []
    bytes_recd = 0
    while bytes_recd < msg_len:
        chunk = sock.recv(min(msg_len - bytes_recd, 1024 * 64))
        if not chunk:
            break
        chunks.append(chunk)
        bytes_recd += len(chunk)
        
    data = b"".join(chunks)
    response_str = data.decode('utf-8')
    
    # Parse Envelope
    try:
        wrapper = json.loads(response_str)
        if wrapper.get("status") == "success":
            return wrapper.get("result")
        else:
            print(f"Server Error: {wrapper.get('message')}")
            return None
    except json.JSONDecodeError:
        print(f"Invalid JSON: {response_str}")
        return None

def main():
    print("--- PCG Client Test ---")
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10.0)
            s.connect((HOST, PORT))
            print("Connected to Unreal Server.")
            
            # 1. Create Graph
            print("\n1. Creating PCG Graph...")
            pkg_path = "/Game/Data/PCG_Test"
            asset_name = f"PCG_SocketTest_{int(time.time())}"
            
            res_create = send_request(s, {
                "command": "create_pcg_graph",
                "package_path": pkg_path,
                "asset_name": asset_name
            })
            
            if res_create and "path" in res_create:
                graph_path = res_create["path"]
                print(f"[SUCCESS] Created Graph: {graph_path}")
                
                # 2. Add Node A
                print("\n2. Adding StaticMeshSpawner Node at (250, 150)...")
                res_add = send_request(s, {
                    "command": "add_pcg_node",
                    "graph_path": graph_path,
                    "node_class": "PCGStaticMeshSpawnerSettings",
                    "position_x": 250,
                    "position_y": 150
                })
                
                if res_add and "node_name" in res_add:
                    node_a_name = res_add['node_name']
                    print(f"[SUCCESS] Added Node A: {node_a_name}")
                    
                    # 3. Add Node B
                    print("\n3. Adding Node B (Spawner) at (550, 150)...")
                    res_b = send_request(s, {
                        "command": "add_pcg_node",
                        "graph_path": graph_path,
                        "node_class": "PCGStaticMeshSpawnerSettings",
                        "position_x": 550,
                        "position_y": 150
                    })
                    
                    if res_b and "node_name" in res_b:
                        node_b_name = res_b['node_name']
                        print(f"[SUCCESS] Added Node B: {node_b_name}")
                        
                        # 4. Connect A -> B
                        print("\n4. Connecting A -> B...")
                        res_conn = send_request(s, {
                            "command": "connect_pcg_nodes",
                            "graph_path": graph_path,
                            "upstream_node": node_a_name,
                            "downstream_node": node_b_name,
                            "upstream_pin": "Out",
                            "downstream_pin": "In"
                        })
                        print(f"Connection Result: {res_conn}")
                        
                        # 5. Set Property (Debug & Mesh)
                        print("\n5. Setting Properties on Node A (Debug & Mesh)...")
                        
                        # Use a standard engine cube for testing
                        test_mesh_path = "/Engine/BasicShapes/Cube.Cube"
                        
                        res_prop = send_request(s, {
                            "command": "set_pcg_node_properties",
                            "graph_path": graph_path,
                            "node_name": node_a_name,
                            "properties": {
                                "debug": True,
                                "MeshEntries": [test_mesh_path]
                            }
                        })
                        print(f"Property Result: {res_prop}")
                        
                    else:
                        print("[FAIL] Failed to add Node B")
                else:
                    print(f"[FAIL] Failed to add node. Response: {res_add}")
                    
            else:
                print("[FAIL] Failed to create graph.")
                
    except Exception as e:
        print(f"Connection Error: {e}")

if __name__ == "__main__":
    main()
