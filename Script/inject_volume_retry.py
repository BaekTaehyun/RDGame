import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Volume Sampler Injection (Retry) ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 2. Add 'Create Points Grid' Node
    print("Creating Volume Sampler...")
    
    # add_node_of_type returns a Tuple?? Let's check
    res_tuple = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
    print(f"Result Tuple: {res_tuple}")
    
    source_node = None
    if isinstance(res_tuple, tuple) or isinstance(res_tuple, list):
         # Try to find the Node object in the tuple
         for item in res_tuple:
             if hasattr(item, 'position_x'):
                 source_node = item
                 break
    else:
        source_node = res_tuple
        
    if not source_node:
        print("ERROR: Could not retrieve Node from return value.")
    else:
        print(f"Source Node Created: {source_node}")
        source_node.position_x = -1000
        source_node.position_y = -500
        source_node.node_title = "DEBUG_VOLUME_SOURCE"
        
        # Configure Source
        s_settings = source_node.get_settings()
        try:
            s_settings.set_editor_property("grid_extents", unreal.Vector(5000, 5000, 5000))
            s_settings.set_editor_property("cell_size", unreal.Vector(500, 500, 500))
            s_settings.set_editor_property("cull_points_outside_volume", False) # Unbounded
            s_settings.set_editor_property("unbounded", True)
            print("Grid Settings Configured")
        except: pass

        # 3. Add FRESH Spawner Node
        print("Creating Fresh Spawner...")
        res_tuple2 = graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
        spawner_node = None
        for item in res_tuple2:
             if hasattr(item, 'position_x'):
                 spawner_node = item
                 break
        
        if spawner_node:
            spawner_node.position_x = -500
            spawner_node.position_y = -500
            spawner_node.node_title = "DEBUG_SPAWNER"
            
            # Configure Spawner (Cube)
            sp_settings = spawner_node.get_settings()
            try:
                # Add Mesh Entry via Mesh Selector
                # This is hard via Python dynamically.
                # Instead, try to set the 'Static Mesh' property directly if it's a simple spawner?
                # No, PCG Spawner uses a Selector struct.
                # Let's try to load a known mesh
                mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
                
                # Check properties to see where to inject
                # We will rely on the fact that if this logic runs, 
                # we can ask the USER to assign a mesh to "DEBUG_SPAWNER" manually quickly.
                pass
            except: pass
            
            # 4. Connect Source -> Spawner
            try:
                res = graph.add_edge(source_node, "Out", spawner_node, "In")
                print(f"Connected Volume -> Spawner: {res is not None}")
            except Exception as e:
                print(f"Connection error: {e}")
        
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
                                total += len(pts)
                                if len(pts) > 0:
                                    print(f"Sample Point: {pts[0].transform.translation}")
                    print(f"Total Points: {total}")
            except: pass

print("\\n=== Done ===")
"""

def inject_volume_retry():
    print(f"--- [Inject Volume Sampler Retry] ---", flush=True)
    
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
    inject_volume_retry()
