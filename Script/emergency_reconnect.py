import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] EMERGENCY RECONNECT ---")

graph = unreal.load_asset(graph_path)
if graph:
    grid_node = None
    trans_node = None
    
    # 1. Find Grid
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0":
            grid_node = n
            break
            
    # 2. Find Transform (Any non-Ruins Transform)
    # Ruins usually TransformPoints_2.
    # We want TransformPoints_1 or _3.
    for n in graph.nodes:
        if "TransformPoints" in n.get_name() and n.get_name() != "TransformPoints_2":
            trans_node = n
            print(f"Found Candidate Transform: {n.get_name()}")
            break # Pick first valid
            
    if grid_node and trans_node:
        # 3. Connect Grid -> Transform
        try:
            graph.add_edge(grid_node, "Out", trans_node, "In")
            print("Connected: Grid -> Transform")
        except Exception as e:
            print(f"Grid->Trans Error: {e}")
            
        # 4. Connect Transform -> All Filters
        targets = []
        for i in range(1, 6): # 1 to 5
            targets.append(f"DensityFilter_{i}")
            targets.append(f"AttributeFilter_{i}")
            
        for tname in targets:
            found = None
            for n in graph.nodes:
                if n.get_name() == tname:
                    found = n
                    break
            
            if found:
                try:
                    graph.add_edge(trans_node, "Out", found, "In")
                    print(f"Connected: Transform -> {tname}")
                except Exception as e:
                    print(f"Trans->{tname} Error: {e}")
                    
    else:
        print("Critical: Missing Grid or Transform nodes.")
        if not grid_node: print("  Grid Missing!")
        if not trans_node: print("  Transform Missing!")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Reconnect Attempt Complete.")

"""

def emergency_reconnect():
    print(f"--- [Fix] Reconnecting ---", flush=True)
    
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
    emergency_reconnect()
