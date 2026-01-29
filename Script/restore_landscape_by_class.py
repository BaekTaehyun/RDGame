import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Restore: Landscape By Class ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    # 1. Cleanup Debug Nodes
    nodes_to_remove = []
    
    landscape_node = None
    sampler_node = None
    transform_node = None
    spawner_node = None
    
    for node in graph.nodes:
        t = str(node.node_title)
        s = node.get_settings()
        if not s: continue
        c = s.get_class().get_name()
        
        if "DEBUG" in t or "CreatePointsGrid" in c or "WorldRayHit" in c or "Projection" in c:
            nodes_to_remove.append(node)
        elif "GetLandscape" in c:
            landscape_node = node
        elif "SurfaceSampler" in c:
            sampler_node = node
        elif "TransformPoints" in c:
            transform_node = node
        elif "StaticMeshSpawner" in c:
            spawner_node = node
            
    for n in nodes_to_remove:
        try:
             graph.remove_node(n)
        except: pass
    print(f"Cleaned {len(nodes_to_remove)} debug nodes.")
    
    # 2. Configure Landscape Node (By Class)
    if landscape_node:
        try:
            settings = landscape_node.get_settings()
            selector = settings.get_editor_property("actor_selector")
            
            # EPCGActorSelection Enum:
            # ByTag = 0
            # ByName = 1
            # ByClass = 2
            
            selector.set_editor_property("actor_selection", unreal.PCGActorSelection.BY_CLASS)
            # Find Landscape Class
            selector.set_editor_property("actor_selection_class", unreal.Landscape)
            print("Set Landscape Selector to: BY CLASS (Landscape)")
        except Exception as e:
            print(f"Config Error: {e}")
            
    # 3. Configure Sampler (Ensure Unbounded)
    if sampler_node:
        try:
            settings = sampler_node.get_settings()
            settings.set_editor_property("unbounded", True)
            print("Set Sampler: Unbounded=True")
        except: pass

    # 4. Connect Chain
    if landscape_node and sampler_node and transform_node and spawner_node:
        try:
            # Landscape -> Sampler
            graph.add_edge(landscape_node, "Out", sampler_node, "Surface")
            
            # Sampler -> Transform
            graph.add_edge(sampler_node, "Out", transform_node, "In")
            
            # Transform -> Spawner
            graph.add_edge(transform_node, "Out", spawner_node, "In")
            
            print("Connected Chain: Landscape -> Sampler -> Transform -> Spawner")
            
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

def restore_landscape_by_class():
    print(f"--- [Restore Landscape By Class] ---", flush=True)
    
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
    restore_landscape_by_class()
