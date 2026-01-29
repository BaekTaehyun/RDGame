import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

# This script will:
# 1. Remove ALL existing edges from the graph
# 2. Create a clean simple chain: Landscape -> Sampler -> Transform -> Spawner

PYTHON_CODE = """
import unreal

print("=== Clean PCG Graph Rebuild ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # Find key nodes
    landscape_node = None
    sampler_node = None
    transform_node = None
    spawner_nodes = []
    filter_nodes = []
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        cname = settings.get_class().get_name()
        
        if "GetLandscape" in cname:
            landscape_node = node
        elif "SurfaceSampler" in cname:
            sampler_node = node
        elif "TransformPoints" in cname:
            transform_node = node
        elif "StaticMeshSpawner" in cname:
            spawner_nodes.append(node)
        elif "DensityFilter" in cname:
            filter_nodes.append(node)
    
    print(f"Landscape: {landscape_node is not None}")
    print(f"Sampler: {sampler_node is not None}")
    print(f"Transform: {transform_node is not None}")
    print(f"Spawners: {len(spawner_nodes)}")
    print(f"Filters: {len(filter_nodes)}")
    
    # Clear all existing edges first
    print("\\n=== Clearing Existing Edges ===")
    try:
        # Try remove_all_edges
        graph.remove_all_edges()
        print("All edges removed!")
    except Exception as e:
        print(f"remove_all_edges failed: {e}")
        # Try alternative: remove edges one by one
        # or just proceed with adding new ones (they should override)
    
    # Build clean chain
    print("\\n=== Building Clean Chain ===")
    
    # Step 1: Landscape -> Sampler (Surface input)
    if landscape_node and sampler_node:
        try:
            result = graph.add_edge(landscape_node, "Out", sampler_node, "Surface")
            print(f"Landscape -> Sampler (Surface): {result is not None}")
        except Exception as e:
            print(f"Error: {e}")
    
    # Step 2: Sampler -> Transform
    if sampler_node and transform_node:
        try:
            result = graph.add_edge(sampler_node, "Out", transform_node, "In")
            print(f"Sampler -> Transform: {result is not None}")
        except Exception as e:
            print(f"Error: {e}")
    
    # Step 3: Transform -> First Spawner directly (skip filters for now to debug)
    if transform_node and spawner_nodes:
        first_spawner = spawner_nodes[0]
        try:
            result = graph.add_edge(transform_node, "Out", first_spawner, "In")
            print(f"Transform -> Spawner[0]: {result is not None}")
        except Exception as e:
            print(f"Error: {e}")
    
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("\\nGraph Saved!")
    
    # Now trigger regeneration
    print("\\n=== Triggering Regeneration ===")
    world = unreal.EditorLevelLibrary.get_editor_world()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        comps = actor.get_components_by_class(unreal.PCGComponent)
        for comp in comps:
            comp.set_graph(graph)
            comp.generate_local(True)
            print(f"Regenerated: {comp.get_name()}")

print("\\n=== Done ===")
"""

def rebuild_graph():
    print(f"--- [Clean PCG Graph Rebuild] ---", flush=True)
    
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
    rebuild_graph()
