import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def inspect_restored_nodes():
    print(f"--- [Debug] Inspecting Ruins Nodes ---", flush=True)
    
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
        req = {"jsonrpc": "2.0", "method": method, "params": params}
        if expect_response: req["id"] = req_id
        try:
            proc.stdin.write(json.dumps(req) + "\n")
            proc.stdin.flush()
        except: return None
        if expect_response: return json.loads(proc.stdout.readline())
        return None

    try:
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)
        
        # 1. Check Filter
        print("Checking DensityFilter_0...")
        res = rpc("tools/call", {
            "name": "get_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "DensityFilter_0",
                "property_names": ["LowerBound", "UpperBound"]
            }
        })
        print(f"Filter: {res['result']['content'][0]['text']}")

        # 2. Check Spawner
        print("\nChecking StaticMeshSpawner_4...")
        res = rpc("tools/call", {
            "name": "get_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "StaticMeshSpawner_4",
                "property_names": ["MeshEntries"]
            }
        })
        print(f"Spawner: {res['result']['content'][0]['text']}")

    except Exception as e:
        print(f"[Error] {e}")
    finally:
        proc.terminate()

if __name__ == "__main__":
    inspect_restored_nodes()
