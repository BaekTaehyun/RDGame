import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Restore Landscape Sampling ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Identify Nodes
    volume_node = None
    landscape_node = None
    sampler_node = None
    transform_node = None
    spawner_node = None
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        cname = settings.get_class().get_name()
        
        if "CreatePointsGrid" in cname or node.node_title.startswith("DEBUG"):
            volume_node = node
        elif "GetLandscape" in cname:
            landscape_node = node
        elif "SurfaceSampler" in cname:
            sampler_node = node
        elif "TransformPoints" in cname:
            transform_node = node
        elif "StaticMeshSpawner" in cname:
            # Pick the one connected to volume if possible, or just the first one
            # We want the MAIN spawner
            spawner_node = node

    # 2. Cleanup Debug Node
    # Can't remove, but we can disconnect it
    if volume_node:
        print(f"Disconnecting/Ignoring Debug Node: {volume_node.node_title}")
        # Ideally we remove edges from it?
        # graph.remove_edge? No API.
        # Effectively we just won't use it.
    
    # 3. Restore Chain: Landscape -> Sampler -> Transform -> Spawner
    if landscape_node and sampler_node and transform_node and spawner_node:
        print("Restoring Connection Chain...")
        
        try:
            # 3.1 Landscape -> Sampler
            # Note: Input pin name might be 'Surface' or 'In'
            # Sampler input pin for Surface is usually 'Surface'
            graph.add_edge(landscape_node, "Out", sampler_node, "Surface")
            print("  Connected: Landscape -> Sampler")
            
            # 3.2 Sampler -> Transform
            graph.add_edge(sampler_node, "Out", transform_node, "In")
            print("  Connected: Sampler -> Transform")
            
            # 3.3 Transform -> Spawner
            graph.add_edge(transform_node, "Out", spawner_node, "In")
            print("  Connected: Transform -> Spawner")
            
            # 4. Verify/Set Sampler Settings Again
            sampler_settings = sampler_node.get_settings()
            sampler_settings.set_editor_property("unbounded", True)
            sampler_settings.set_editor_property("looseness", 1.0)
            print("  Surface Sampler: Unbounded=True, Looseness=1.0")
            
        except Exception as e:
            print(f"Connection Error: {e}")
            
    else:
        print("Error: Missing core nodes (Landscape/Sampler/Transform/Spawner)")

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
            
            # Check Output
            try:
                data = comp.get_generated_graph_output()
                if data:
                    total = 0
                    if hasattr(data, 'tagged_data'):
                        for td in data.tagged_data:
                            if hasattr(td.data, 'get_points'):
                                pts = td.data.get_points()
                                count = len(pts)
                                total += count
                                if count > 0:
                                     print(f"  First Point: {pts[0].transform.translation}")
                    print(f"Total Points: {total}")
            except: pass

print("\\n=== Done ===")
"""

def restore_landscape():
    print(f"--- [Restore Landscape Sampling] ---", flush=True)
    
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
    restore_landscape()
