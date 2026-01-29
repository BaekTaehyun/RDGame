import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Volume Sampler Injection (True Debug) ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Clear All Edges
    # We can't remove_all_edges easily, but we can just ignore old nodes.
    # We will remove edges from our target spawner if possible?
    # No, let's just create a FRESH pair of nodes to be safe.
    
    # 2. Add 'Create Points Grid' Node
    print("Creating Volume Sampler...")
    # NOTE: verify class name
    source_node = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
    source_node.position_x = -1000
    source_node.position_y = -500
    source_node.node_title = "DEBUG_VOLUME_SOURCE"
    
    # Configure Source
    s_settings = source_node.get_settings()
    # Set HUGE Extents
    try:
        s_settings.set_editor_property("grid_extents", unreal.Vector(5000, 5000, 5000))
        s_settings.set_editor_property("cell_size", unreal.Vector(500, 500, 500))
        # Cull points outside volume? If False, it generates everywhere?
        # Usually checking 'Cull Points Outside Volume' = False makes it Unbounded?
        # Let's try Unbounded if available
        try:
            s_settings.set_editor_property("unbounded", True)
        except: pass
    except Exception as e:
        print(f"Source config error: {e}")

    # 3. Add FRESH Spawner Node
    print("Creating Fresh Spawner...")
    spawner_node = graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
    spawner_node.position_x = -500
    spawner_node.position_y = -500
    spawner_node.node_title = "DEBUG_SPAWNER"
    
    # Configure Spawner
    sp_settings = spawner_node.get_settings()
    try:
        # Load a mesh
        mesh_path = "/Engine/BasicShapes/Cube.Cube" # Use Cube first to be sure
        mesh = unreal.load_asset(mesh_path)
        
        # Method A: Mesh Selector Instance
        selector = sp_settings.get_editor_property("mesh_selector_instance")
        if selector:
            # We need to add an entry
            # Usually struct PCGStaticMeshSpawnerEntry
            entry = unreal.PCGStaticMeshSpawnerEntry()
            entry.set_editor_property("mesh", mesh)
            entry.set_editor_property("weight", 1)
            
            # Add to array
            # This part is tricky via Python (Array of Structs).
            # Let's try direct assignment if property exists
            try:
                # Some versions expose 'Meshes' directly on settings or selector
                pass
            except: pass
            
    except Exception as e:
        print(f"Spawner config error: {e}")
        
    # If we can't set mesh easily, we will try to connect to the EXISTING Spawner
    # find existing spawner
    target_spawner = None
    for node in graph.nodes:
        s = node.get_settings()
        if s and "StaticMeshSpawner" in s.get_class().get_name():
            # Use one that is likely configured
            if node != spawner_node:
                target_spawner = node
                break
    
    final_spawner = target_spawner if target_spawner else spawner_node
    print(f"Target Spawner: {final_spawner.get_name()}")
    
    # 4. Connect Source -> Spawner
    try:
        res = graph.add_edge(source_node, "Out", final_spawner, "In")
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
                                total += len(td.data.get_points())
                    print(f"Total Points: {total}")
            except: pass

print("\\n=== Done ===")
"""

def inject_volume_sampler():
    print(f"--- [Inject Volume Sampler] ---", flush=True)
    
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
    inject_volume_sampler()
