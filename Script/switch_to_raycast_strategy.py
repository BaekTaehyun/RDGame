import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Switch to Robust Physics Raycast Strategy ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    # 1. Cleanup Old Logic (Sampler/Landscape/Debug)
    nodes_to_remove = []
    
    # Existing valid nodes we want to keep?
    # We want to keep Transform and Spawner.
    transform_node = None
    spawner_node = None
    
    for node in graph.nodes:
        t = str(node.node_title)
        c = node.get_settings().get_class().get_name()
        
        # Keep Transform and Spawner
        if "TransformPoints" in c: 
            transform_node = node
            continue
        if "StaticMeshSpawner" in c: 
            spawner_node = node
            continue
            
        # Remove Logic Nodes (Landscape, Sampler) and Debug Nodes
        if "GetLandscape" in c or "SurfaceSampler" in c or "DEBUG" in t or "CreatePointsGrid" in c or "WorldRayHit" in c:
            nodes_to_remove.append(node)
            
    for n in nodes_to_remove:
        try: graph.remove_node(n)
        except: pass
        
    print(f"Removed {len(nodes_to_remove)} old nodes.")
        
    # 2. Add 'Points Grid' (Huge Volume)
    res = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
    grid_node = res[0]
    grid_set = res[1]
    grid_node.node_title = "ROBUST_GRID"
    
    # Set to cover huge area (100km)
    try:
        # Extents are Half-Size. 50000 = 1km diameter? No 500m radius.
        # Landscape was ~630m. Let's do 100000 (1km radius, 2km width).
        grid_set.set_editor_property("grid_extents", unreal.Vector(100000, 100000, 100000))
        # Density (Cell Size). Lower = More Dense. 
        # Previous 0.1/m2 -> ~3.16m spacing. 
        # let's try 400 (4m spacing).
        grid_set.set_editor_property("cell_size", unreal.Vector(400, 400, 1000)) 
        grid_set.set_editor_property("cull_points_outside_volume", False)
        # Cull points inside actor bounds? No.
        print("Created Huge Grid (100k extent)")
    except Exception as e:
        print(f"Error setting grid: {e}")
        
    # 3. Add 'World Ray Hit' (The Physics Projector)
    res2 = graph.add_node_of_type(unreal.PCGWorldRayHitSettings)
    ray_node = res2[0]
    ray_set = res2[1]
    ray_node.node_title = "ROBUST_RAY"
    
    try:
        # Ray Direction: -Z (Down) is default.
        # Ray Length: Infinite or huge.
        ray_set.set_editor_property("ray_length", 1000000.0)
        # Collision Channel: WorldStatic
        # ray_set.set_editor_property("collision_channel", ...) default is usually visibility or WorldStatic
    except: pass
    
    # 4. Connect Chain
    if grid_node and ray_node and transform_node:
        # Grid -> Ray
        graph.add_edge(grid_node, "Out", ray_node, "In")
        # Ray -> Transform
        graph.add_edge(ray_node, "Out", transform_node, "In")
        print("Connected Chain: Grid -> Ray -> Transform -> Spawner")
    else:
        print("ERROR: Missing nodes for connection")

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved.")
    
    # Regen
    world = unreal.EditorLevelLibrary.get_editor_world()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if "Anchor" in actor.get_actor_label():
            comps = actor.get_components_by_class(unreal.PCGComponent)
            if comps:
                comps[0].generate_local(True)
                print(f"Regenerated {actor.get_name()}")

print("\\n=== Done ===")
"""

def switch_to_raycast_strategy():
    print(f"--- [Switch to Raycast Strategy] ---", flush=True)
    
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
    switch_to_raycast_strategy()
