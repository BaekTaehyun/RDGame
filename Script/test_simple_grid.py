import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Debug] Simple Grid Bypass ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Create Test Grid
    grid_node = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)[0]
    grid_node.set_node_position(0, -500)
    
    try:
        s = grid_node.get_settings()
        # World Space = 0? Or 1?
        # CoordinateSpace: 0=World?, 1=Original?
        # Let's try to set GridExtents large
        s.set_editor_property("GridExtents", unreal.Vector(2500, 2500, 200))
        s.set_editor_property("CellSize", unreal.Vector(100, 100, 100))
        
        # Force World Space if posssible
        # If not sure about Enum, default is usually World/0.
    except: pass
    
    # 2. Find Spawner
    spawner_node = None
    for n in graph.nodes:
        if "StaticMeshSpawner" in n.get_name():
            spawner_node = n
            break # Just take the first one
            
    # 3. Connect Grid -> Spawner
    if spawner_node:
        print(f"Connecting Test Grid -> {spawner_node.get_name()}")
        try:
            graph.add_edge(grid_node, "Out", spawner_node, "In")
        except Exception as e:
            print(f"Conn Error: {e}")
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    
    # Sync
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def test_bypass():
    print(f"--- [Debug] Bypass ---", flush=True)
    
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
    test_bypass()
