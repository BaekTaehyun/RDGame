import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def analyze_pcg_cloud():
    print(f"--- [Analysis] Inspecting PCG Graph: {GRAPH_PATH} ---", flush=True)
    
    # 1. Start Bridge
    print("[1/5] Starting MCP Bridge...", flush=True)
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr, # Allow bridge logs to show in console
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
        req_id = int(time.time()*1000)
        req = {"jsonrpc": "2.0", "method": method, "params": params, "id": req_id}
        
        # print(f"  -> Sending {method}...", flush=True)
        json_str = json.dumps(req)
        try:
            proc.stdin.write(json_str + "\n")
            proc.stdin.flush()
        except Exception as e:
            print(f"[Error] Failed to write to bridge: {e}", flush=True)
            return None

        if expect_response:
            # print("  <- Waiting for response...", flush=True)
            line = proc.stdout.readline()
            if not line:
                print("[Error] Bridge stdout closed unexpectedly.", flush=True)
                return None
            return json.loads(line)
        else:
            # print("  (Notification sent, no response expected)", flush=True)
            return None

    try:
        # Initialize
        print("[2/5] Initializing...", flush=True)
        rpc("initialize", {}, expect_response=True)
        # Notification - DO NOT WAIT
        rpc("notifications/initialized", {}, expect_response=False)

        # 2. Get Topology
        print("[3/5] Fetching Graph Topology (may take a moment)...", flush=True)
        res_inspect = rpc("tools/call", {
            "name": "inspect_pcg_graph",
            "arguments": {"graph_path": GRAPH_PATH}
        })
        
        if not res_inspect or "result" not in res_inspect:
            print(f"[Error] Inspect failed: {res_inspect}", flush=True)
            return

        content = res_inspect["result"]["content"][0]["text"]
        topo_raw = json.loads(content)
        
        nodes_list = []
        if "data" in topo_raw:
            # C++ Data (Stringified JSON)
            nodes_list = json.loads(topo_raw["data"])["Nodes"]
        elif "nodes" in topo_raw:
            # Python Data
            nodes_list = topo_raw["nodes"]
            
        print(f"  -> Found {len(nodes_list)} Nodes in Graph.", flush=True)

        # 3. Identify Targets
        targets = []
        for n in nodes_list:
            name = n.get("Name", "")
            title = n.get("Title", "")
            full_str = f"{name}|{title}"
            
            if "Grid" in full_str:
                targets.append({"name": name, "type": "Grid", "props": ["CellSize"]})
            elif "Transform" in full_str:
                targets.append({"name": name, "type": "Transform", "props": ["offset_min", "offset_max", "rotation_max", "scale_min", "scale_max"]})
            elif "Ruins" in full_str and "Var" in full_str: 
                targets.append({"name": name, "type": "RuinsVar", "props": ["offset_max", "rotation_max", "scale_max"]})
            elif "Filter" in full_str and "Ruins" in full_str:
                targets.append({"name": name, "type": "RuinsFilter", "props": ["LowerBound", "UpperBound"]})

        # 4. Fetch Properties
        print(f"[4/5] Analyzing Settings for {len(targets)} Key Nodes...", flush=True)
        
        print("\n--- [Analysis Report] ---")
        for t in targets:
            # Shorten name for display
            disp_name = t['name']
            
            res_prop = rpc("tools/call", {
                "name": "get_pcg_node_properties",
                "arguments": {
                    "graph_path": GRAPH_PATH,
                    "node_name": t["name"],
                    "property_names": t["props"]
                }
            })
            
            p_data = json.loads(res_prop["result"]["content"][0]["text"])
            
            print(f"> Node: {disp_name} ({t['type']})")
            if p_data.get("status") == "success":
                props = p_data.get("properties", {})
                for k, v in props.items():
                    print(f"   - {k}: {v}", flush=True)
            else:
                print(f"   [Error] {p_data.get('error')}", flush=True)
            print("") # Newline

        print("[5/5] Analysis Complete.", flush=True)

    except Exception as e:
        print(f"[Fatal Error] Script Exception: {e}", flush=True)
    finally:
        proc.terminate()
        # print("\n--- Finished ---", flush=True)

if __name__ == "__main__":
    analyze_pcg_cloud()
