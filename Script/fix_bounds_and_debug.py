import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Inject Bounds Modifier (Invalid Bounds Fix) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Nodes
    wall_node = None
    copy_node = None
    bounds_node = None
    
    for n in graph.nodes:
        nm = n.get_name()
        if "CopyPoints" in nm: copy_node = n
        if "BoundsModifier" in nm: bounds_node = n
        if "DungeonDataReader" in nm:
            t = ""
            try: t = n.get_editor_property("NodeTitleOverride")
            except: pass
            if "Wall" in t or "2" in nm: wall_node = n
            
    # 2. Create BoundsModifier if missing
    if not bounds_node:
        bounds_node = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)[0]
        bounds_node.set_node_position(-350, 100)
        print("Created BoundsModifier.")
        
    # 3. Configure Bounds
    # Set to +/- 200 to give volume to the points
    try:
        s = bounds_node.get_settings()
        v = unreal.Vector(200, 200, 200)
        s.set_editor_property("BoundsMin", v.get_reversed()) # -200
        s.set_editor_property("BoundsMax", v)
        # Enable Debug
        bounds_node.set_editor_property("bDebug", True)
    except Exception as e:
        print(f"Bounds Config Error: {e}")
        
    # 4. Connect Chain: Wall -> Bounds -> Copy(Target)
    if wall_node and copy_node and bounds_node:
        # Connect Wall -> Bounds
        try: graph.add_edge(wall_node, "Out", bounds_node, "In")
        except: pass
        
        # Connect Bounds -> Copy (Target)
        try: graph.add_edge(bounds_node, "Out", copy_node, "Target")
        except: pass
        
        print("Connected: Wall -> Bounds -> Copy(Target)")
    else:
        print("Missing required nodes for wiring.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    
    # Enable Debug on Copy Node too
    if copy_node: copy_node.set_editor_property("bDebug", True)

    # Sync
    try:
        unreal.DungeonAssetUtils.refresh_blueprint(graph)
        print("Graph Refreshed.")
    except Exception as e:
        print(f"Sync Error: {e}")
"""

def fix_bounds():
    print(f"--- [Fix] Bounds ---", flush=True)
    
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
