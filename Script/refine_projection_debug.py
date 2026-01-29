import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Refined Projection Debug ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    # 1. Cleanup
    nodes_to_remove = []
    for node in graph.nodes:
        t = str(node.node_title)
        c = node.get_settings().get_class().get_name()
        if "DEBUG" in t or "CreatePointsGrid" in c or "WorldRayHit" in c or "Projection" in c:
            nodes_to_remove.append(node)
            
    for n in nodes_to_remove:
        try:
             graph.remove_node(n)
        except: pass
    print(f"Cleaned {len(nodes_to_remove)} debug/old nodes.")
    
    # 2. Inject Huge Grid
    grid_res = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
    grid_node = grid_res[0]
    grid_set = grid_res[1]
    grid_node.node_title = "DEBUG_GRID"
    try:
        grid_set.set_editor_property("grid_extents", unreal.Vector(25000, 25000, 25000)) # 250m box
        grid_set.set_editor_property("cell_size", unreal.Vector(500, 500, 500))
        grid_set.set_editor_property("cull_points_outside_volume", False)
    except: pass
    
    # 3. Inject Ray Hit (Long)
    ray_res = graph.add_node_of_type(unreal.PCGWorldRayHitSettings)
    ray_node = ray_res[0]
    ray_set = ray_res[1]
    ray_node.node_title = "DEBUG_RAY"
    try:
        ray_set.set_editor_property("ray_length", 1000000.0) # Infinite
        ray_set.set_editor_property("ignore_self_hits", True)
        # ray_set.set_editor_property("collision_channel", ...) # Default is WorldStatic
    except: pass
    
    # 4. Inject Projection
    proj_res = graph.add_node_of_type(unreal.PCGProjectionSettings)
    proj_node = proj_res[0]
    proj_set = proj_res[1]
    proj_node.node_title = "DEBUG_PROJ"
    
    # 5. Connect
    transform_node = None
    spawner_node = None
    for n in graph.nodes:
        if "TransformPoints" in n.get_settings().get_class().get_name():
            transform_node = n
        if "StaticMeshSpawner" in n.get_settings().get_class().get_name():
            spawner_node = n
            
    if transform_node and spawner_node:
        try:
            # Grid -> Proj
            graph.add_edge(grid_node, "Out", proj_node, "In")
            
            # Ray -> Proj (Try mulitple pin names for Target)
            # Usually 'Projection Target'
            graph.add_edge(ray_node, "Out", proj_node, "Projection Target")
            
            # Proj -> Transform
            graph.add_edge(proj_node, "Out", transform_node, "In")
            
            # Transform -> Spawner
            graph.add_edge(transform_node, "Out", spawner_node, "In")
            
            print("Connected Refined Chain: Grid(Huge) + Ray(1M) -> Proj -> Transform -> Spawner")
            
        except Exception as e:
            print(f"Connection Error: {e}")
            
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved.")
    
    # Regen
    world = unreal.EditorLevelLibrary.get_editor_world()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            comps[0].generate_local(True)
            print("Regenerated.")
            
print("\\n=== Done ===")
"""

def refine_projection_debug():
    print(f"--- [Refine Projection Debug] ---", flush=True)
    
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
    refine_projection_debug()
