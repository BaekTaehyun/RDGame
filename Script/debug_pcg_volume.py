import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Debug: Test Volume Sampler (Create Points Grid) ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Clear Nodes
    # NOTE: Python API cannot easily DELETE nodes. 
    # But we can create a NEW node and connect IT instead.
    
    # Create 'Create Points Grid' node
    print("Creating 'Create Points Grid' node...")
    grid_node = graph.add_node(unreal.PCGCreatePointsGridSettings)
    grid_node.node_title = "Debug_Grid_Source"
    grid_node.position_x = -500
    grid_node.position_y = 0
    
    # Configure Grid Settings
    settings = grid_node.get_settings()
    try:
        settings.set_editor_property("grid_extents", unreal.Vector(1000, 1000, 1000))
        settings.set_editor_property("cell_size", unreal.Vector(100, 100, 100))
        settings.set_editor_property("cull_points_outside_volume", False)
        print("Grid Settings Configured")
    except Exception as e:
        print(f"Grid Settings Error: {e}")
        
    # Find Transform Node
    transform_node = None
    for node in graph.nodes:
        s = node.get_settings()
        if s and "TransformPoints" in s.get_class().get_name():
            transform_node = node
            break
    
    if transform_node:
        # Connect Grid -> Transform
        # Note: CreatePointsGrid output is "Out"
        try:
            res = graph.add_edge(grid_node, "Out", transform_node, "In")
            print(f"Connected Grid -> Transform: {res is not None}")
        except Exception as e:
            print(f"Connection Error: {e}")
    else:
        print("Error: Transform Node not found")

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved!")
    
    # Trigger Regen
    world = unreal.EditorLevelLibrary.get_editor_world()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            comps[0].generate_local(True)
            print(f"Regenerated {comps[0].get_name()}")
            
            # Check Output
            try:
                data = comps[0].get_generated_graph_output()
                if data and hasattr(data, 'tagged_data'):
                    print(f"Output Tagged Data Count: {len(data.tagged_data)}")
            except: pass

print("\\n=== Done ===")
"""

def debug_with_volume():
    print(f"--- [Debug PCG with Volume] ---", flush=True)
    
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
    debug_with_volume()
