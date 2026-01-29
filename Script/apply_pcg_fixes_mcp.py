import sys
import json
import subprocess
import time
import socket

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

# Verified Names from Step 5373
NODE_TREE_TRANSFORM = "TransformPoints_1"
NODE_RUINS_FILTER = "DensityFilter_1"
NODE_RUINS_VAR = "TransformPoints_0"
NODE_RUINS_SPAWNER = "StaticMeshSpawner_4"
NODE_SOURCE = "SelfPruning_0"

def fix_pcg_final():
    print(f"--- [Fixing] Applying Visual Fixes via MCP ---", flush=True)
    
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
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        # 1. Fix Tree Rotation (Yaw 360)
        print("[1/4] Fixing Tree Rotation (Yaw 360)...", flush=True)
        rpc("tools/call", {
            "name": "set_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": NODE_TREE_TRANSFORM,
                "properties": {
                    "rotation_max": {"pitch": 0.0, "yaw": 360.0, "roll": 0.0},
                    "uniform_scale": True
                }
            }
        }, True)

        # 2. Configure Ruins Transform
        print("[2/4] Configuring Ruins Transform...", flush=True)
        rpc("tools/call", {
            "name": "set_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": NODE_RUINS_VAR,
                "properties": {
                    "rotation_max": {"pitch": 0.0, "yaw": 360.0, "roll": 0.0},
                    "scale_min": {"x": 2.5, "y": 2.5, "z": 2.5},
                    "scale_max": {"x": 4.5, "y": 4.5, "z": 4.5},
                    "uniform_scale": True
                }
            }
        }, True)
        
        # 3. Force Connect (Using Verified Names)
        print("[3/4] Wiring Ruins Chain...", flush=True)
        
        # SelfPruning -> Ruins_Filter
        rpc("tools/call", {
            "name": "connect_pcg_nodes",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "upstream_node": NODE_SOURCE,
                "downstream_node": NODE_RUINS_FILTER
            }
        }, True)
        
        # Ruins_Filter -> Ruins_Variator
        rpc("tools/call", {
            "name": "connect_pcg_nodes",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "upstream_node": NODE_RUINS_FILTER,
                "downstream_node": NODE_RUINS_VAR
            }
        }, True)

        # Ruins_Variator -> Spawner
        res_conn = rpc("tools/call", {
            "name": "connect_pcg_nodes",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "upstream_node": NODE_RUINS_VAR,
                "downstream_node": NODE_RUINS_SPAWNER
            }
        }, True)
        print(f"   Last Link Result: {res_conn['result']['content'][0]['text']}", flush=True)

        # 4. Save
        print("[4/4] Saving Asset...", flush=True)
        code_save = f"""
import unreal
unreal.EditorAssetLibrary.save_asset("{GRAPH_PATH}")
print("Saved!")
        """
        rpc("tools/call", {
            "name": "execute_unreal_script",
            "arguments": {"code": code_save}
        }, True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()
        print("\n--- Fixes Complete ---", flush=True)

if __name__ == "__main__":
    fix_pcg_final()
