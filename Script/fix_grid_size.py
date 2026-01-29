import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Fix Grid Size (Match Landscape) ===")

# 1. Get Landscape Bounds
world = unreal.EditorLevelLibrary.get_editor_world()
landscapes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)

if not landscapes:
    print("ERROR: No Landscape found (using fallback 5000)")
    bounds_extent = unreal.Vector(5000, 5000, 5000)
    origin = unreal.Vector(0,0,0)
else:
    land = landscapes[0]
    # get_actor_bounds(boolean bOnlyCollidingComponents) -> (Origin, BoxExtent)
    origin, extent = land.get_actor_bounds(False)
    print(f"Landscape Detected: {land.get_name()}")
    print(f"  - Origin: {origin}")
    print(f"  - Extent: {extent}")
    bounds_extent = extent

    # Check if bounds are suspiciously small (e.g. empty landscape)
    if bounds_extent.x < 100:
       print("WARNING: Bounds too small, using fallback 5000")
       bounds_extent = unreal.Vector(5000, 5000, 5000)

# 2. Update Graph
graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if graph:
    grid_node = None
    for node in graph.nodes:
        if "ROBUST_GRID" in str(node.node_title):
            grid_node = node
            break
            
    if grid_node:
        settings = grid_node.get_settings()
        
        # Set Extents to match Landscape 
        # Add 10% margin
        safe_extent = unreal.Vector(
            bounds_extent.x * 1.1,
            bounds_extent.y * 1.1,
            100000 # Z can be tall for rays
        )
        settings.set_editor_property("grid_extents", safe_extent)
        
        # Verify Cell Size (Density)
        # 400 = 4 meters.
        settings.set_editor_property("cell_size", unreal.Vector(400, 400, 1000))
        
        print(f"Updated ROBUST_GRID Extents to: {safe_extent}")
        
        unreal.EditorAssetLibrary.save_loaded_asset(graph)
        print("Graph Saved.")
        
        # Regen
        for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
            if "Anchor" in actor.get_actor_label():
                comps = actor.get_components_by_class(unreal.PCGComponent)
                if comps:
                    comps[0].generate_local(True)
                    print(f"Regenerated {actor.get_name()}")
    else:
        print("ERROR: ROBUST_GRID node not found!")
else:
    print("ERROR: Graph not loaded")

print("\\n=== Done ===")
"""

def fix_grid_size():
    print(f"--- [Fix Grid Size] ---", flush=True)
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
        req = {"jsonrpc": "2.0", "method": method, "params": params}
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
    fix_grid_size()
