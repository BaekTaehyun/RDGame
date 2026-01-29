import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Diagnosis] Inspecting & Bypassing ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Check Grid Settings
    grid_node = None
    ruins_grid = None
    
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0": grid_node = n
        # Ruins grid found by class check earlier
        s = n.get_settings()
        if s and "PCGCreatePointsGridSettings" in s.get_class().get_name() and n.get_name() != "CreatePointsGrid_0":
             ruins_grid = n

    if grid_node:
        try:
            s = grid_node.get_settings()
            # Check CellSize
            val = s.get_editor_property("CellSize")
            print(f"Forest Grid CellSize: {val}")
        except Exception as e:
            print(f"Grid Property Error: {e}")
            
    if ruins_grid:
        try:
            s = ruins_grid.get_settings()
            val = s.get_editor_property("CellSize")
            print(f"Ruins Grid CellSize: {val}")
        except: pass

    # 2. Bypass Transform (Direct Connection)
    # We want to enable visualizing SOMETHING.
    # Connect Grid -> DensityFilter_1 (Big Trees)
    # Connect Grid -> DensityFilter_2 (Medium)
    
    # Try multiple Pin Name combinations because I suspect "Out" might be wrong for Grid.
    # Actually, standard PCG nodes use "Out" and "In".
    
    print("Attempting Direct Connection (Bypass Transform)...")
    
    filter_1 = None
    for n in graph.nodes:
        if n.get_name() == "DensityFilter_1": filter_1 = n
    
    if grid_node and filter_1:
        try:
            graph.add_edge(grid_node, "Out", filter_1, "In")
            print("Connected: Grid(Out) -> Filter_1(In)")
        except Exception as e:
            print(f"Connect Error: {e}")
            
    # Also Bypass Ruins
    ruins_filter = None
    for n in graph.nodes:
        if n.get_name() == "DensityFilter_5": ruins_filter = n
        
    if ruins_grid and ruins_filter:
        try:
            graph.add_edge(ruins_grid, "Out", ruins_filter, "In")
            print("Connected: RuinsGrid(Out) -> RuinsFilter(In)")
        except: pass

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Bypass Applied. Check Viewport.")
    
"""

def diagnose_blackout():
    print(f"--- [Diagnosis] Running ---", flush=True)
    
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
    diagnose_blackout()
