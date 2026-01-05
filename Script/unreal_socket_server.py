import unreal
import socket
import json
import threading
import queue
import time
import traceback
import sys
import os
import base64 # Added for thumbnail encoding

# Ensure script directory (where test_asset_scanner.py lives) is in path
SCRIPT_DIR = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script"
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)

# --- Config ---
HOST = '127.0.0.1'
PORT = 3001

# --- Global State ---
command_queue = queue.Queue()
response_queue = queue.Queue()
server_running = False
tick_handle = None

def log(msg):
    print(f"[UnrealMCP] {msg}")

# --- Worker Thread (Socket) ---
def socket_worker():
    global server_running
    
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Allow port reuse
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind((HOST, PORT))
        server_socket.listen(1)
        log(f"Listening on {HOST}:{PORT}...")
        
        while server_running:
            # Blocking accept
            # To stop smoothly, we might need a timeout or connect from main thread
            server_socket.settimeout(1.0) 
            try:
                conn, addr = server_socket.accept()
            except socket.timeout:
                continue
                
            log(f"Connected by {addr}")
            
            with conn:
                while server_running:
                    # Simple length-prefixed or line-based protocol?
                    # For simplicity, let's assume one-shot connection or line-based for now.
                    # Let's use standard line-based JSON for simple testing.
                    try:
                        data = conn.recv(1024*64) # 64KB buffer
                        if not data:
                            break
                        
                        message = data.decode('utf-8')
                        log(f"Received: {message}")
                        
                        # Request logic from Main Thread
                        req_id = id(message) # simple ID
                        
                        # Put into command queue for Main Thread
                        command_queue.put({
                            'id': req_id,
                            'raw': message,
                            'conn': conn # dangerous to use conn across threads? 
                                         # sending from this thread is fine.
                        })
                        
                        # Wait for response (Blocking this thread)
                        # In a real async server, we wouldn't block here.
                        # But for a simple synchronous MCP bridge, this is fine multiple clients not expected.
                        
                        while server_running:
                            try:
                                res = response_queue.get(timeout=0.1)
                                if res['id'] == req_id:
                                    # Send Length-Prefixed Response
                                    # 4 bytes big-endian length + payload
                                    payload_bytes = res['data'].encode('utf-8')
                                    length_bytes = len(payload_bytes).to_bytes(4, 'big')
                                    
                                    log(f"Sending response ID:{req_id} ({len(payload_bytes)} bytes)...")
                                    conn.sendall(length_bytes + payload_bytes)
                                    log("Response Sent.")
                                    break
                                else:
                                    # Oops, wrong response order? (Should not happen with 1 client)
                                    # response_queue.put(res) 
                                    pass # Debug: Drop mismatch to see if that's the issue? No, re-queue.
                                    response_queue.put(res)
                            except queue.Empty:
                                continue
                                
                    except Exception as e:
                        log(f"Connection Error: {e}")
                        traceback.print_exc() # Log full trace
                        break
                        
    except Exception as e:
        log(f"Server Error: {e}")
    finally:
        server_socket.close()
        log("Server Socket Closed")

# --- Main Thread (Unreal Tick) ---
def unreal_tick(delta_seconds):
    # Process all pending commands
    while not command_queue.empty():
        cmd_wrapper = command_queue.get()
        req_id = cmd_wrapper['id']
        raw_msg = cmd_wrapper['raw']
        
        log(f"Main Thread Processing: {raw_msg[:50]}...") # Debug log
        
        response_data = ""
        
        try:
            # Parse JSON
            payload = json.loads(raw_msg)
            command = payload.get("command")
            
            result = None
            
            # --- Command Handling ---
            if command == "scan_assets":
                path = payload.get("path", "/Game")
                import test_asset_scanner # Import dynamically to allow reloading
                # log(f"Executing scan_assets on {path}")
                result = test_asset_scanner.scan_assets_lightweight(path)

                
            elif command == "get_thumbnail":
                asset_path = payload.get("asset_path")
                if not asset_path:
                    result = {"error": "Missing asset_path"}
                else:
                    # Use our C++ BPL
                    try:
                        if hasattr(unreal, "DungeonAssetUtils"):
                            png_bytes = unreal.DungeonAssetUtils.capture_thumbnail(asset_path)
                            if len(png_bytes) > 0:
                                raw_bytes = bytes(png_bytes)
                                b64_str = base64.b64encode(raw_bytes).decode('utf-8')
                                result = {"asset_path": asset_path, "image_data": b64_str, "format": "png"}
                            else:
                                result = {"error": "Thumbnail capture returned empty", "asset_path": asset_path}
                        else:
                            result = {"error": "DungeonAssetUtils plugin not available"} 
                    except Exception as e:
                        result = {"error": f"Capture failed: {str(e)}"}
            
            # --- PCG Commands ---
            elif command == "create_pcg_graph":
                package_path = payload.get("package_path") # e.g. /Game/PCG
                asset_name = payload.get("asset_name")     # e.g. MyPCG
                
                asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
                factory = unreal.PCGGraphFactory()
                # Create
                new_asset = asset_tools.create_asset(asset_name, package_path, unreal.PCGGraph, factory)
                if new_asset:
                    result = {"status": "created", "path": new_asset.get_path_name()}
                else:
                    result = {"error": "Failed to create PCG Graph asset"}

            elif command == "add_pcg_node":
                graph_path = payload.get("graph_path")
                node_class_name = payload.get("node_class") # e.g. PCGStaticMeshSpawnerSettings
                
                # Load Graph
                graph_asset = unreal.load_asset(graph_path)
                if not graph_asset or not isinstance(graph_asset, unreal.PCGGraph):
                    result = {"error": f"Invalid graph path: {graph_path}"}
                else:
                    # Resolve Class
                    node_class = getattr(unreal, node_class_name, None)
                    if not node_class:
                        result = {"error": f"Unknown PCGSettings class: {node_class_name}"}
                    else:
                        try:
                            # Add Node
                            # returns (Node, Settings) or similar tuple
                            ret_val = graph_asset.add_node_of_type(node_class)
                            
                            new_node = None
                            if isinstance(ret_val, (tuple, list)):
                                new_node = ret_val[0]
                            else:
                                new_node = ret_val
                                
                            if new_node:
                                # Set Position if provided
                                pos_x = payload.get("position_x")
                                pos_y = payload.get("position_y")
                                if pos_x is not None and pos_y is not None:
                                    try:
                                        # Important: Mark as modified for transaction/update
                                        new_node.modify()
                                        log(f"Setting position to {pos_x}, {pos_y}")
                                        new_node.set_node_position(int(pos_x), int(pos_y))
                                    except Exception as e_pos:
                                        log(f"Failed to set position: {e_pos}")

                                # Safe name retrieval
                                node_name = new_node.get_name()
                                node_title = "Unknown"
                                if hasattr(new_node, "node_title"):
                                     try:
                                         node_title = str(new_node.node_title)
                                     except:
                                         pass
                                         
                                result = {
                                    "status": "added", 
                                    "node_name": node_name
                                }
                            else:
                                result = {"error": "add_node_of_type returned None"}
                        except Exception as e:
                             traceback.print_exc()
                             result = {"error": f"Exception adding node: {str(e)}"}

            elif command == "connect_pcg_nodes":
                graph_path = payload.get("graph_path")
                up_name = payload.get("upstream_node")
                down_name = payload.get("downstream_node")
                up_pin = payload.get("upstream_pin", "Out")
                down_pin = payload.get("downstream_pin", "In")
                
                graph_asset = unreal.load_asset(graph_path)
                if not graph_asset:
                    result = {"error": "Graph not found"}
                else:
                    # Find Nodes
                    up_node = None
                    down_node = None
                    
                    # Iterate nodes (graph.nodes is a list/array)
                    all_nodes = graph_asset.nodes
                    for n in all_nodes:
                        n_name = n.get_name()
                        if n_name == up_name:
                            up_node = n
                        if n_name == down_name:
                            down_node = n
                            
                    if up_node and down_node:
                        try:
                            # add_edge_to(from_label, to_node, to_label)
                            up_node.add_edge_to(up_pin, down_node, down_pin)
                            result = {"status": "connected"}
                        except Exception as e:
                            result = {"error": f"Connection failed: {e}"}
                    else:
                        result = {"error": f"Nodes not found. Up:{up_node is not None}, Down:{down_node is not None}"}

            elif command == "set_pcg_node_properties":
                graph_path = payload.get("graph_path")
                node_name = payload.get("node_name")
                props = payload.get("properties", {})
                
                graph_asset = unreal.load_asset(graph_path)
                if not graph_asset:
                     result = {"error": "Graph not found"}
                else:
                    target_node = None
                    for n in graph_asset.nodes:
                        if n.get_name() == node_name:
                            target_node = n
                            break
                    
                    if target_node:
                        settings = target_node.get_settings()
                        if settings:
                            successes = []
                            errors = []
                            
                            # TYPE CONVERSION HELPER
                            def convert_to_unreal(val):
                                if isinstance(val, dict):
                                    keys = val.keys()
                                    # Vector
                                    if 'x' in keys and 'y' in keys and 'z' in keys:
                                        return unreal.Vector(val['x'], val['y'], val['z'])
                                    # Rotator
                                    if 'roll' in keys and 'pitch' in keys and 'yaw' in keys:
                                        return unreal.Rotator(val['pitch'], val['yaw'], val['roll'])
                                    # LinearColor
                                    if 'r' in keys and 'g' in keys and 'b' in keys:
                                        return unreal.LinearColor(val['r'], val['g'], val['b'], val.get('a', 1.0))
                                return val

                            # Helper for smart setting
                            def set_smart_property(obj, p_name, p_val):
                                # Convert generic JSON dicts to Unreal types if needed
                                safe_val = convert_to_unreal(p_val)

                                # 1. Try Direct Property
                                try:
                                    obj.set_editor_property(p_name, safe_val)
                                    return True
                                except:
                                    pass

                                # 2. Check for Mesh Selector (Indirection)
                                selector = getattr(obj, "mesh_selector_parameters", None)
                                if not selector:
                                    selector = getattr(obj, "mesh_selector_instance", None)
                                
                                if selector:
                                    try:
                                        # Special Case: MeshEntries from List of Strings
                                        if p_name == "MeshEntries" and isinstance(p_val, list):
                                            # Valid list check
                                            if len(p_val) > 0 and isinstance(p_val[0], str):
                                                # 1. Get Entry Class (Verified)
                                                entry_cls = getattr(unreal, "PCGMeshSelectorWeightedEntry", None)
                                                if not entry_cls:
                                                    # Fallback or error
                                                    pass

                                                entries = []
                                                for path in p_val:
                                                    mesh_asset = unreal.load_asset(path)
                                                    if mesh_asset:
                                                        # Create Entry
                                                        new_entry = entry_cls()
                                                        
                                                        # 2. Get Descriptor (Struct - Value Type)
                                                        try:
                                                            desc = new_entry.get_editor_property("Descriptor")
                                                            
                                                            # 3. Set StaticMesh (Try Common Names)
                                                            set_mesh = False
                                                            for mesh_prop in ["StaticMesh", "Mesh"]:
                                                                try:
                                                                    desc.set_editor_property(mesh_prop, mesh_asset)
                                                                    set_mesh = True
                                                                    break
                                                                except:
                                                                    pass
                                                            
                                                            if set_mesh:
                                                                # 4. Write Entry Back (Struct pass-by-value)
                                                                new_entry.set_editor_property("Descriptor", desc)
                                                                
                                                                # 5. Set Weight (Default 1)
                                                                new_entry.set_editor_property("Weight", 1)
                                                                
                                                                entries.append(new_entry)
                                                        except Exception as e_desc:
                                                            print(f"Failed to set descriptor: {e_desc}")
                                                
                                                # Update Selector
                                                selector.set_editor_property("MeshEntries", entries)
                                                return True
                                        
                                        # Normal set on selector
                                        selector.set_editor_property(p_name, p_val)
                                        return True
                                    except Exception as e_sel:
                                        # print(f"Selector set failed: {e_sel}")
                                        pass
                                        
                                return False

                            for p_name, p_val in props.items():
                                try:
                                    if set_smart_property(settings, p_name, p_val):
                                         successes.append(p_name)
                                    else:
                                         errors.append(f"{p_name}: Failed to find/set property")
                                except Exception as e:
                                    errors.append(f"{p_name}: {e}")
                            
                            result = {"status": "processed", "success": successes, "errors": errors}
                        else:
                            result = {"error": "Node has no settings"}
                    else:
                        result = {"error": "Node not found"}

            elif command == "ping":
                result = "pong"
                
            else:
                result = {"error": "Unknown command"}
                
            response_data = json.dumps({"status": "success", "result": result})
            
        except Exception as e:
            traceback.print_exc()
            response_data = json.dumps({"status": "error", "message": str(e)})
            
        # Send back to thread
        response_queue.put({
            'id': req_id,
            'data': response_data
        })
    
    return True # Continue ticking

# --- Lifecycle ---
def start_server():
    global server_running, tick_handle
    
    if server_running:
        log("Server already running. Stopping first...")
        stop_server()
        
    log("Starting Server...")
    server_running = True
    
    # Start Thread
    t = threading.Thread(target=socket_worker)
    t.daemon = True
    t.start()
    
    # Register Tick
    tick_handle = unreal.register_slate_post_tick_callback(unreal_tick)
    log("Tick Registered")

def stop_server():
    global server_running, tick_handle
    
    log("Stopping Server...")
    server_running = False
    
    if tick_handle:
        unreal.unregister_slate_post_tick_callback(tick_handle)
        tick_handle = None
        log("Tick Unregistered")
        
    # Give thread time to close socket
    time.sleep(0.5)

if __name__ == "__main__":
    # If run as script, just start
    start_server()
