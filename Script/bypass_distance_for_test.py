import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Debug] Bypass Distance (Test Grid) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {}
    for n in graph.nodes:
        nm = n.get_name()
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "TransformPoints" in nm: nodes["Trans"] = n
        if "Distance" in nm: nodes["Dist"] = n
        
    if nodes.get("Grid") and nodes.get("Trans"):
        # Explicitly connect Grid -> Trans
        # This bypasses Distance.
        try:
            graph.add_edge(nodes["Grid"], "Out", nodes["Trans"], "In")
            print("Connected: Grid -> Transform (Bypassed Distance)")
        except Exception as e:
            print(f"Bypass Error: {e}")
            
    else:
        print("Missing Grid or Transform node.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Bypass Saved.")

"""

def bypass_distance():
    print(f"--- [Debug] Bypassing Distance ---", flush=True)
    
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
    bypass_distance()
