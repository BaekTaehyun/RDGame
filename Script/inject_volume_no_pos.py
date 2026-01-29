import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Volume Sampler Injection (No Pos) ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Create Volume Sampler (Create Points Grid)
    # add_node_of_type returns (Node, Settings)
    res = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
    source_node = res[0]
    source_settings = res[1]
    
    print(f"Node Created: {source_node}")
    
    # Configure Settings
    try:
        source_settings.set_editor_property("grid_extents", unreal.Vector(10000, 10000, 10000))
        source_settings.set_editor_property("cell_size", unreal.Vector(400, 400, 400)) 
        source_settings.set_editor_property("cull_points_outside_volume", False)
        print("Grid Settings Configured")
    except Exception as e:
        print(f"Grid Settings Error (Non-Fatal): {e}")
        
    # 2. Find Existing Spawner
    target_spawner = None
    for node in graph.nodes:
        s = node.get_settings()
        if s and "StaticMeshSpawner" in s.get_class().get_name():
            target_spawner = node
            break
            
    if target_spawner:
        print(f"Connecting to Spawner: {target_spawner.get_name()}")
        
        # 3. Connect
        try:
            # Connect Source "Out" -> Spawner "In"
            graph.add_edge(source_node, "Out", target_spawner, "In")
            print("Connected!")
        except Exception as e:
            print(f"Connection Failed: {e}")
            
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved!")
    
    # Trigger Regen
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
                    print(f"Total Points: {total}")
            except: pass

print("\\n=== Done ===")
"""

def inject_volume_no_pos():
    print(f"--- [Inject Volume No Pos] ---", flush=True)
    
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
    inject_volume_no_pos()
