import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Debug: Revert to Tag + Inject Projection ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    # 1. Cleanup Old Debug Nodes
    nodes_to_remove = []
    landscape_node = None
    transform_node = None
    spawner_node = None
    
    for node in graph.nodes:
        t = str(node.node_title)
        c = node.get_settings().get_class().get_name()
        
        if "DEBUG" in t or "CreatePointsGrid" in c or "WorldRayHit" in c or "Projection" in c:
            nodes_to_remove.append(node)
            
        if "GetLandscape" in c: landscape_node = node
        if "TransformPoints" in c: transform_node = node
        if "StaticMeshSpawner" in c: spawner_node = node
            
    for n in nodes_to_remove:
        try: graph.remove_node(n)
        except: pass
        
    # 2. Revert Landscape to By Tag
    if landscape_node:
        try:
            settings = landscape_node.get_settings()
            selector = settings.get_editor_property("actor_selector")
            selector.set_editor_property("actor_selection", unreal.PCGActorSelection.BY_TAG)
            selector.set_editor_property("actor_selection_tag", "DungeonGeneratedLandscape")
            print("Set Landscape Selector: BY TAG (DungeonGeneratedLandscape)")
        except: pass
        
    # 3. Inject Projection Chain (Parallel Test)
    # We will connect this INSTEAD of Landscape for now to guarantee output if collision works.
    
    # Grid
    res = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
    grid_node = res[0]
    grid_set = res[1]
    grid_node.node_title = "DEBUG_GRID"
    try:
        grid_set.set_editor_property("grid_extents", unreal.Vector(25000, 25000, 25000))
        grid_set.set_editor_property("cell_size", unreal.Vector(500, 500, 500))
        grid_set.set_editor_property("cull_points_outside_volume", False)
    except: pass
    
    # Ray
    res2 = graph.add_node_of_type(unreal.PCGWorldRayHitSettings)
    ray_node = res2[0]
    ray_set = res2[1]
    ray_node.node_title = "DEBUG_RAY"
    try:
        ray_set.set_editor_property("ray_length", 1000000.0)
    except: pass
    
    # Proj
    res3 = graph.add_node_of_type(unreal.PCGProjectionSettings)
    proj_node = res3[0]
    proj_node.node_title = "DEBUG_PROJ"
    
    # Connect Chain
    if grid_node and ray_node and proj_node and transform_node:
        # Grid -> Proj
        graph.add_edge(grid_node, "Out", proj_node, "In")
        # Ray -> Proj
        graph.add_edge(ray_node, "Out", proj_node, "Projection Target")
        
        # Proj -> Transform (Disconnect Landscape for now to isolate)
        # We need to find the edge from Sampler to Transform and break it?
        # Actually add_edge might allow multiple inputs or not. Transform usually allows multiple.
        # But let's try to ensure ONLY Projection feeds Transform to be sure.
        
        # Break existing inputs to Transform
        # Using Python API to break edges is hard ("remove_edge" needs pointers).
        # We'll just add the edge. Transform usually Unions inputs.
        
        graph.add_edge(proj_node, "Out", transform_node, "In")
        print("Connected Projection Debug Chain")

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved.")
    
    # Regen
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if "Anchor" in actor.get_actor_label():
            comps = actor.get_components_by_class(unreal.PCGComponent)
            if comps:
                comps[0].generate_local(True)
                print(f"Regenerated {actor.get_name()}")

print("\\n=== Done ===")
"""

def debug_projection_anchor():
    print(f"--- [Debug Projection Anchor] ---", flush=True)
    
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
    debug_projection_anchor()
