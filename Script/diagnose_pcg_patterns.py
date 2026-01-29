import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def diagnose_patterns():
    print(f"--- [Diagnosis] Checking Ruins Configuration & Tree Jitter ---", flush=True)
    
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
        if expect_response: return json.loads(proc.stdout.readline())
        return None

    try:
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        # 1. Check Ruins Filter (DensityFilter_1)
        print("[1/3] Reading Ruins Filter Settings...", flush=True)
        res_filter = rpc("tools/call", {
            "name": "get_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "DensityFilter_1",
                "property_names": ["LowerBound", "UpperBound"]
            }
        }, True)
        print(f"   Filter: {res_filter['result']['content'][0]['text']}", flush=True)

        # 2. Check Ruins Spawner (StaticMeshSpawner_4)
        print("[2/3] Reading Ruins Meshes...", flush=True)
        res_spawner = rpc("tools/call", {
            "name": "get_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "StaticMeshSpawner_4",
                "property_names": ["MeshEntries", "StaticMeshComponentPropertyOverrides"]
            }
        }, True)
        # Note: MeshEntries might be complex struct, server prints basic info?
        print(f"   Spawner: {res_spawner['result']['content'][0]['text']}", flush=True)

        # 3. Check Tree Jitter (TransformPoints_1)
        print("[3/3] Reading Tree Jitter (Offset)...", flush=True)
        res_tree = rpc("tools/call", {
            "name": "get_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "TransformPoints_1",
                "property_names": ["offset_min", "offset_max"]
            }
        }, True)
        print(f"   Tree Offset: {res_tree['result']['content'][0]['text']}", flush=True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    diagnose_patterns()
