import sys
import json
import subprocess
import time
import socket

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def check_server_alive(host='127.0.0.1', port=3001, timeout=2.0):
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False

def verify_pcg_state():
    print(f"--- [Verification] Checking Ruins Chain & Settings ---", flush=True)
    
    if not check_server_alive():
        print("[Error] Unreal Socket Server is NOT accessible.", flush=True)
        return

    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
        req_id = int(time.time()*1000)
        req = {"jsonrpc": "2.0", "method": method, "params": params, "id": req_id}
        try:
            proc.stdin.write(json.dumps(req) + "\n")
            proc.stdin.flush()
        except: return None

        if expect_response:
            return json.loads(proc.stdout.readline())
        return None

    try:
        # 1. Initialize
        print("[1/3] Initializing Bridge...", flush=True)
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        # 2. Fetch Topology
        print("[2/3] Fetching Graph Topology...", flush=True)
        res = rpc("tools/call", {
            "name": "inspect_pcg_graph",
            "arguments": {"graph_path": GRAPH_PATH}
        }, True)
        
        topo_raw = json.loads(res["result"]["content"][0]["text"])
        nodes = []
        if "data" in topo_raw:
             nodes = json.loads(topo_raw["data"])["Nodes"] 
        elif "nodes" in topo_raw:
             nodes = topo_raw["nodes"]

        # 3. Connection Check (Using Exact Names)
       # Define Expected Connections
    # (Upstream, Downstream)
    expected_edges = [
        ("SelfPruning_0", "DensityFilter_5"),
        ("DensityFilter_5", "TransformPoints_2"),
        ("TransformPoints_2", "StaticMeshSpawner_5")
    ]
    
    # We can't access edges directly easily in Python without iterating pins.
    # But we can try to use 'get_inputs/outputs' if available or use the C++ helper if server is up.
    # Assuming server is up (User restarted Editor -> Server OFF).
    # So we must use pure Python Pin Inspection.
    
    print("--- [Audit] Verifying Edges (Python) ---")
    # Python API for Pins is tricky.
    # Let's try to assume if nodes exist, we just force connect them again to be safe?
    # Or try to read 'DownstreamNodes' if exposed?
    
    # The original script does not define 'graph' or import 'unreal'.
    # The provided snippet seems to be for an Unreal Editor Python script,
    # not for this external verification script.
    # To make the change syntactically correct and functional within the existing
    # script's context (which uses RPC calls to an external bridge),
    # I will adapt the intent to verify the chain using the existing node topology
    # and RPC mechanism, rather than attempting direct Unreal API calls.

    # Re-implementing the connection check based on the new node names
    print("\n--- Connection Check (Ruins) ---", flush=True)
    node_map = {
        "SelfPruning": "SelfPruning_0",
        "Ruins_Filter": "DensityFilter_5",
        "Ruins_Variator": "TransformPoints_2",
        "Spawner_Ruins": "StaticMeshSpawner_5"
    }
    
    chain = ["SelfPruning", "Ruins_Filter", "Ruins_Variator", "Spawner_Ruins"]
    prev_node = None
    
    for key in chain:
        target_name = node_map.get(key, key)
        curr_node = None
        for n in nodes:
            if n["Name"] == target_name:
                curr_node = n
                break
        
        if not curr_node:
            print(f" [FAIL] Node missing: {target_name} ({key})", flush=True)
            prev_node = None
            continue
            
        name = curr_node["Name"]
        status = "START"
        if prev_node:
            outbound = prev_node.get("Outbound", [])
            is_connected = False
            for out_n in outbound:
                if out_n == name:
                    is_connected = True
                    break
                outbound = prev_node.get("Outbound", [])
                is_connected = False
                for out_n in outbound:
                    if out_n == name:
                        is_connected = True
                        break
                status = "LINKED" if is_connected else "BROKEN"
                print(f"  |-> [{status}] to {name}", flush=True)
            
            print(f" [Node] {name}", flush=True)
            prev_node = curr_node

        # 4. Settings Check
        print("\n--- Rotation Check ---", flush=True)
        tree_node_name = "TransformPoints_1" # Trees
        
        res_prop = rpc("tools/call", {
            "name": "get_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": tree_node_name,
                "property_names": ["rotation_max"]
            }
        }, True)
        
        p_text = res_prop["result"]["content"][0]["text"]
        props = json.loads(p_text).get("properties", {})
        rot = props.get("rotation_max", {})
        
        p = rot.get('pitch', 0)
        y = rot.get('yaw', 0)
        
        print(f" Tree Rotation: Pitch={p}, Yaw={y} (Z-Axis)", flush=True)
        
        if y > 300 and (p < 10 or p > 350): # Allow close to 0
             if p > 300: # 360 case
                  print("   [INFO] Pitch is 360 (Effective 0). Yaw is High. OK.", flush=True)
             else:
                  print("   [OK] Correct Z-Axis Spin. Pitch is Zero.", flush=True)
        elif p > 300:
             print("   [FAIL] Pitch is High, Yaw is Low? Check values.", flush=True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    verify_pcg_state()
