import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Debug: Grid -> Projection ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Create Grid Node
    res = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
    grid_node = res[0]
    grid_settings = res[1]
    
    grid_node.node_title = "DEBUG_GRID"
    grid_node.position_x = -1500
    grid_node.position_y = -300
    
    # Settings (Huge Volume)
    try:
        grid_settings.set_editor_property("grid_extents", unreal.Vector(10000, 10000, 10000))
        grid_settings.set_editor_property("cell_size", unreal.Vector(500, 500, 500))
        grid_settings.set_editor_property("cull_points_outside_volume", False)
    except: pass
    
    # 2. Create Projection Node
    res2 = graph.add_node_of_type(unreal.PCGProjectionSettings)
    proj_node = res2[0]
    proj_settings = res2[1]
    
    proj_node.node_title = "DEBUG_PROJECTION"
    proj_node.position_x = -1000
    proj_node.position_y = -300
    
    # Settings (Project Down)
    try:
        # Default might be 'Project Z' which is what we want.
        # Check defaults if possible, or force Project Target
        # By default PCGProjection projects Input to Target.
        # If Target is not connected, does it default to World?
        # Usually we need to check 'Projection Target' property?
        # Actually, let's look for 'ProjectionMode' if exists?
        print("Configuring Projection...")
        # Often projection target needs to be connected. 
        # But if we want World Raycast, we might need 'WorldRayHit' data?
        # Let's try connecting just In -> Out first and see if default works (often Self).
        pass
    except: pass
    
    # 3. Create World Ray Hit Node? 
    # To project against World, we usually need 'World Ray Hit' data as Target?
    # Or just use Projection settings? 
    # Let's try to add 'PCGWorldRayHitSettings' node and connect to Target.
    res3 = graph.add_node_of_type(unreal.PCGWorldRayHitSettings)
    ray_node = res3[0]
    ray_node.node_title = "DEBUG_RAY"
    ray_node.position_x = -1500
    ray_node.position_y = 0
    
    # 4. Find Transform & Spawner
    transform_node = None
    spawner_node = None
    for node in graph.nodes:
        if "TransformPoints" in node.get_settings().get_class().get_name():
            transform_node = node
        if "StaticMeshSpawner" in node.get_settings().get_class().get_name():
            spawner_node = node
            
    # 5. Connect Chain:
    # Grid -> Projection(In)
    # Ray -> Projection(Target)
    # Projection -> Transform -> Spawner
    
    if grid_node and proj_node and ray_node and transform_node and spawner_node:
        try:
            graph.add_edge(grid_node, "Out", proj_node, "In")
            graph.add_edge(ray_node, "Out", proj_node, "Target") # Pin might be 'Projection Target'
            # Note: Pin names are tricky. Usually 'In' and 'Target'.
            
            graph.add_edge(proj_node, "Out", transform_node, "In")
            graph.add_edge(transform_node, "Out", spawner_node, "In")
            
            print("Connected DEBUG Chain: Grid + Ray -> Projection -> Transform -> Spawner")
        except Exception as e:
            print(f"Connection Error: {e}")
            
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved!")
    
    # Regen
    world = unreal.EditorLevelLibrary.get_editor_world()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            comp = comps[0]
            comp.generate_local(True)
            print(f"Regenerated {comp.get_name()}")
            
print("\\n=== Done ===")
"""

def debug_projection():
    print(f"--- [Debug Projection] ---", flush=True)
    
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
    debug_projection()
