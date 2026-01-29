import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Bypassing Ruins Logic ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Create dedicated Grid Source
    # Check if we already made one
    ruins_grid = None
    df = None
    
    for n in graph.nodes:
        name = n.get_name()
        title = "Unknown"
        try: title = n.get_editor_property("NodeTitleOverride").__str__()
        except: pass
        
        if "Ruins_Source" in title:
            ruins_grid = n
        if name == "DensityFilter_5":
            df = n
            
    if not df:
        print("Error: DensityFilter_5 not found. Cannot bypass.")
    else:
        if not ruins_grid:
            # Create New
            ret = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
            ruins_grid = ret[0]
            
            try:
                ruins_grid.set_editor_property("NodeTitleOverride", "Ruins_Source_Grid")
                # Position it left of Filter
                px = df.get_editor_property("NodePosX")
                py = df.get_editor_property("NodePosY")
                ruins_grid.set_node_position(px - 300, py)
            except: pass
            
            # Configure Grid
            settings = ruins_grid.get_settings()
            try:
                # Set Cell Size to something large so they aren't too dense
                settings.set_editor_property("CellSize", unreal.Vector(500, 500, 100))
                # CullBackface?
                settings.cull_backface_normals = False
            except Exception as e:
                print(f"Grid Settings Error: {e}")
        
        # 2. Connect Grid -> Filter
        # This effectively replaces the SelfPruning connection
        try:
             graph.add_edge(ruins_grid, "Out", df, "In")
             print("Connected: Ruins_Source_Grid -> DensityFilter_5")
             
             unreal.EditorAssetLibrary.save_loaded_asset(graph)
             print("Bypass Successful. Dedicated Grid created for Ruins.")
        except Exception as e:
             print(f"Connection Error: {e}")

"""

def bypass_logic():
    print(f"--- [Fix] Isolating Ruins ---", flush=True)
    
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
    bypass_logic()
