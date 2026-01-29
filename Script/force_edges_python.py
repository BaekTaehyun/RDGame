import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] Enforcing Edges (Python) ---")

graph = unreal.load_asset(graph_path)
if graph:
    spf = None
    df = None
    tp = None
    sp = None
    
    # 1. Find Nodes
    for n in graph.nodes:
        name = n.get_name()
        if name == "SelfPruning_0": spf = n
        elif name == "DensityFilter_5": df = n
        elif name == "TransformPoints_2": tp = n
        elif name == "StaticMeshSpawner_5": sp = n
        
    # 2. Add Edges (Idempotent usually)
    if spf and df and tp and sp:
        try:
             # Force Connect
             # Notes: add_edge(UpNode, UpPinLabel, DownNode, DownPinLabel)
             # Labels: "Out" / "In" are standard for PCG.
             
             graph.add_edge(spf, "Out", df, "In")
             print("Connected: SelfPruning -> DensityFilter_5")
             
             graph.add_edge(df, "Out", tp, "In")
             print("Connected: DensityFilter -> Transform")
             
             graph.add_edge(tp, "Out", sp, "In")
             print("Connected: Transform -> Spawner")
             
             unreal.EditorAssetLibrary.save_loaded_asset(graph)
             print("Graph Saved with Enforced Edges.")
        except Exception as e:
             print(f"Connection Error: {e}")
    else:
        print(f"Missing Nodes: SPF={spf!=None}, DF={df!=None}, TP={tp!=None}, SP={sp!=None}")

"""

def force_edges():
    print(f"--- [Audit] Enforcing Connections ---", flush=True)
    
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
    force_edges()
