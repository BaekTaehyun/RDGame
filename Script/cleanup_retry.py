import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Cleanup & Restore (Retry) ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Identify Nodes
    debug_nodes = []
    landscape_node = None
    sampler_node = None
    transform_node = None
    spawner_node = None
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        cname = settings.get_class().get_name()
        
        # FIX: Convert node_title (Name) to string
        title = str(node.node_title)
        
        if "CreatePointsGrid" in cname or title.startswith("DEBUG"):
            debug_nodes.append(node)
        elif "GetLandscape" in cname:
            landscape_node = node
        elif "SurfaceSampler" in cname:
            sampler_node = node
        elif "TransformPoints" in cname:
            transform_node = node
        elif "StaticMeshSpawner" in cname:
            spawner_node = node
            
    # 2. Cleanup Debug Nodes
    if debug_nodes:
        print(f"Removing {len(debug_nodes)} Debug Nodes...")
        for dn in debug_nodes:
            try:
                graph.remove_node(dn)
                print(f"  Removed {dn.node_title}")
            except Exception as e:
                print(f"  Remove Error: {e}")
    else:
        print("No debug nodes found.")

    # 3. Restore Chain
    if landscape_node and sampler_node and transform_node and spawner_node:
        print("Restoring Landscape Chain...")
        
        try:
            # Landscape -> Sampler
            graph.add_edge(landscape_node, "Out", sampler_node, "Surface")
            
            # Sampler -> Transform
            graph.add_edge(sampler_node, "Out", transform_node, "In")
            
            # Transform -> Spawner
            graph.add_edge(transform_node, "Out", spawner_node, "In")
            
            print("Chain Restored: Landscape -> Sampler -> Transform -> Spawner")
            
             # 4. Verify Sampler Settings
            settings = sampler_node.get_settings()
            settings.set_editor_property("unbounded", True)
            settings.set_editor_property("looseness", 1.0) 
            settings.set_editor_property("points_per_square_meter", 0.05) 
            print("Sampler Settings Verified")
            
        except Exception as e:
            print(f"Connection Error: {e}")
            
    else:
        print("ERROR: Missing Core Nodes!")

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

def cleanup_retry():
    print(f"--- [Cleanup Retry] ---", flush=True)
    
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
    cleanup_retry()
