import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Adjusting Bounds (User Request) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Find Bounds Modifier
    bounds_node = None
    for n in graph.nodes:
        if "BoundsModifier" in n.get_name():
            bounds_node = n
            break
            
    if bounds_node:
        try:
            # User request: "Terrain Width". We interpret as "Large".
            # Setting to 1000 (10 meters radius, 20m box).
            # This ensures standard Grid (Cell 80) fits easily.
            sz = 1000.0
            bounds_node.get_settings().set_editor_property("BoundsMin", unreal.Vector(-sz, -sz, -sz))
            bounds_node.get_settings().set_editor_property("BoundsMax", unreal.Vector(sz, sz, sz))
            print(f"Bounds set to +/- {sz}")
        except Exception as e:
            print(f"Bounds Set Error: {e}")
            
    # 2. Find Grid and ensure it's connected
    # We maintain the bypass logic: Wall -> Bounds -> Grid -> Spawner
    
    grid_node = None
    spawner_node = None
    wall_node = None
    
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_name(): grid_node = n
        if "StaticMeshSpawner_0" in n.get_name(): spawner_node = n
        if "DungeonDataReader" in n.get_name() and "2" in n.get_name(): wall_node = n
        
    if wall_node and bounds_node and grid_node and spawner_node:
        try:
            # Re-enforce connection (Just in case)
            graph.add_edge(wall_node, "Out", bounds_node, "In")
            graph.add_edge(bounds_node, "Out", grid_node, "In")
            graph.add_edge(grid_node, "Out", spawner_node, "In")
            print("Verified Chain: Wall->Bounds(Big)->Grid->Spawner0")
        except: pass

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Bounds Updated.")

"""

def fix_bounds():
    print(f"--- [Fix] Bounds Adjustment ---", flush=True)
    
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
    fix_bounds()
