import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Revert to Native Landscape Strategy ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    # 1. Cleanup Raycast/Grid Nodes
    nodes_to_remove = []
    
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
            
        # Remove Logic Nodes (Ray, Grid, Old Landscape/Sampler if exist to start fresh)
        if "ROBUST" in t or "Grid" in c or "Ray" in c or "GetLandscape" in c or "SurfaceSampler" in c:
            nodes_to_remove.append(node)
            
    for n in nodes_to_remove:
        try: graph.remove_node(n)
        except: pass
        
    print(f"Removed {len(nodes_to_remove)} nodes.")
        
    # 2. Add 'Get Landscape Data'
    res = graph.add_node_of_type(unreal.PCGGetLandscapeSettings)
    land_node = res[0]
    land_set = res[1]
    land_node.node_title = "Landscape_Data"
    
    try:
        # Selector: By Tag "DungeonGeneratedLandscape"
        selector = land_set.get_editor_property("actor_selector")
        selector.set_editor_property("actor_selection", unreal.PCGActorSelection.BY_TAG)
        selector.set_editor_property("actor_selection_tag", "DungeonGeneratedLandscape")
    except: pass
        
    # 3. Add 'Surface Sampler'
    res2 = graph.add_node_of_type(unreal.PCGSurfaceSamplerSettings)
    sampler_node = res2[0]
    sampler_set = res2[1]
    sampler_node.node_title = "Surface_Sampler"
    
    try:
        sampler_set.set_editor_property("unbounded", True)
        # Conservative density
        sampler_set.set_editor_property("points_per_squared_meter", 0.05)
    except: pass
    
    # 4. Connect Chain
    if land_node and sampler_node and transform_node:
        # Landscape -> Sampler
        graph.add_edge(land_node, "Out", sampler_node, "Surface")
        # Sampler -> Transform
        graph.add_edge(sampler_node, "Out", transform_node, "In")
        print("Connected Chain: Landscape -> Sampler -> Transform -> Spawner")
    else:
        print(f"ERROR: Missing nodes. L={land_node}, S={sampler_node}, T={transform_node}")

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved.")
    
    # Regen
    world = unreal.EditorLevelLibrary.get_editor_world()
    count = 0
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if "Anchor" in actor.get_actor_label():
            comps = actor.get_components_by_class(unreal.PCGComponent)
            if comps:
                # Force update bounds just in case? PCG Component usually handles this.
                # But let's verify visual
                comps[0].generate_local(True)
                print(f"Regenerated {actor.get_name()}")
                count += 1
                
    if count == 0:
        print("WARNING: No Anchor Actor found to regen!")

print("\\n=== Done ===")
"""

def revert_to_landscape_native():
    print(f"--- [Revert to Landscape Native] ---", flush=True)
    
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
    revert_to_landscape_native()
