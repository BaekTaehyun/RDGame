import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Optimizing Visual Density ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Optimize Ruins Source (Make sparse)
    ruins_grid = None
    for n in graph.nodes:
        # Check by Class Name string since n.get_settings().get_class() is robust if available
        s = n.get_settings()
        if s and "PCGCreatePointsGridSettings" in s.get_class().get_name():
            name = n.get_name()
            # If it's NOT the original forest grid
            if name != "CreatePointsGrid_0":
                ruins_grid = n
                break
    
    if ruins_grid:
        s = ruins_grid.get_settings()
        try:
             # Increase CellSize to 1500 (15 meters spacing)
             new_size = unreal.Vector(1500, 1500, 200)
             s.set_editor_property("CellSize", new_size)
             print(f"Ruins ({ruins_grid.get_name()}): CellSize -> 1500 (Sparse)")
        except Exception as e:
             print(f"Ruins Set Error: {e}")
    else:
        print("Ruins Grid Not Found via Exclude-Check")

    # 2. Optimize Forest Density (Remove Wall effect)
    # Target Filters 0 and 1 (Small trees/Saplings)
    target_filters = ["DensityFilter_1", "DensityFilter_2"] 
    for fname in target_filters:
        for n in graph.nodes:
            if n.get_name() == fname:
                try:
                    s = n.get_settings()
                    # 0.85 -> Only top 15% spawn. Drastic reduction.
                    try: s.lower_bound = 0.85
                    except: s.set_editor_property("LowerBound", 0.85)
                    print(f"Forest {fname}: LowerBound -> 0.85")
                except: pass

    # 3. Global Grid check (Forest)
    # If 180 was still too dense, push to 250?
    # Let's rely on filter first. 
    
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Optimization Complete.")

"""

def optimize_visuals():
    print(f"--- [Fix] Applying Visual Tweaks ---", flush=True)
    
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
    optimize_visuals()
