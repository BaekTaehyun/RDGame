import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Debug] SANITY CHECK: Direct Connect ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Find Wall Reader (DungeonDataReader_2)
    wall_reader = None
    spawner_0 = None
    
    for n in graph.nodes:
        if n.get_name() == "DungeonDataReader_2": wall_reader = n
        if n.get_name() == "StaticMeshSpawner_0": spawner_0 = n
        
    # 2. Cleanup: Delete Grid (Sky Grid)
    # And delete intermediate connection nodes to clear the path.
    for n in graph.nodes:
        name = n.get_name()
        if "CreatePointsGrid" in name: 
            try: graph.remove_node(n)
            except: pass
            
    # 3. Create Bounds (Essential)
    # We'll use a new one to be sure.
    ret = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)
    bounds = ret[0]
    bounds.set_node_position(200, 0)
    try:
        bounds.get_settings().set_editor_property("BoundsMin", unreal.Vector(-100,-100,-100))
        bounds.get_settings().set_editor_property("BoundsMax", unreal.Vector(100,100,100))
    except: pass

    # 4. DIRECT CONNECT: Reader -> Bounds -> Spawner
    if wall_reader and spawner_0:
        # Clear existing edges on Spawner?
        # Not easy via API, but new edge should work.
        
        graph.add_edge(wall_reader, "Out", bounds, "In")
        graph.add_edge(bounds, "Out", spawner_0, "In")
        
        print("Connected: WallReader -> Bounds -> Spawner_0 (Big Tree).")
        print("Bypassed: Noise, Transform, Project, Filters.")
    else:
        print("Missing Reader or Spawner.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Sanity Check Setup Complete.")

"""

def sanity_check():
    print(f"--- [Debug] Sanity Check ---", flush=True)
    
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
    sanity_check()
