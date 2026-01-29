import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Adjusting Forest Density (Layers 0 & 1) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Adjust Global Grid (Ensure it is large enough)
    grid_node = None
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0":
            grid_node = n
            break
            
    if grid_node:
        settings = grid_node.get_settings()
        try:
            # Enforce 180 (previously 150)
            new_size = unreal.Vector(180, 180, 100) 
            settings.set_editor_property("CellSize", new_size)
            print(f"Updated CellSize to: {new_size}")
        except Exception as e:
            print(f"Grid Error: {e}")

    # 2. Adjust Specific Filters for Layer 0 and 1
    # User feedback: "0 and 1 are too dense"
    target_filters = ["DensityFilter_1", "DensityFilter_2"] 
    
    for fname in target_filters:
        found = False
        for n in graph.nodes:
            if n.get_name() == fname:
                found = True
                try:
                    s = n.get_settings()
                    # Raising LowerBound -> Reduces Count.
                    # 0.7 is robust.
                    try: s.lower_bound = 0.7
                    except: s.set_editor_property("LowerBound", 0.7)
                    
                    print(f"Tightened {fname} LowerBound to 0.7")
                except Exception as e:
                     print(f"Filter Error {fname}: {e}")
                break
        if not found:
            print(f"Warning: {fname} not found")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Forest Layers 0 & 1 Thinned.")

"""

def fix_density():
    print(f"--- [Fix] Reducing Density ---", flush=True)
    
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
    fix_density()
