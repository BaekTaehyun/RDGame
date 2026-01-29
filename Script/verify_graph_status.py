import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Status] Inspecting Graph Connections ---")

graph = unreal.load_asset(graph_path)
if graph:
    print(f"Graph: {graph.get_name()}")
    num_nodes = len(graph.nodes)
    print(f"Node Count: {num_nodes}")
    
    # Analyze Edges
    # API: graph.get_all_edges() ? No. 
    # We have to inspect nodes or infer from behavior?
    # Inspect Node Inputs/Outputs? 
    # 'PCGGraph' doesn't easily expose edges list in Python API v1 without helper.
    # BUT, we can try to re-connect and see if it says "Already connected"?
    # Or just Dump Node names.
    
    nodes_found = []
    for n in graph.nodes:
        nodes_found.append(n.get_name())
        
    print(f"Nodes: {nodes_found}")
    
    # Check key nodes existence
    has_grid = any("CreatePointsGrid" in n for n in nodes_found)
    has_dist = any("Distance" in n for n in nodes_found)
    has_trans = any("Transform" in n for n in nodes_found)
    
    print(f"Has Grid: {has_grid}")
    print(f"Has Distance: {has_dist}")
    print(f"Has Transform: {has_trans}")

"""

def verify_status():
    print(f"--- [Status] Verifying ---", flush=True)
    
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

        res = rpc("tools/call", {
            "name": "execute_unreal_script",
            "arguments": {"code": PYTHON_CODE}
        }, True)
        
        print(res.get('result', {}).get('content', [{'text': 'No Output'}])[0]['text'])

    except Exception as e:
        print(f"[Error] {e}")
    finally:
        proc.terminate()

if __name__ == "__main__":
    verify_status()
