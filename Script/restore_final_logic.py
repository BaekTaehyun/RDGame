import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Restore Final Native Logic (Surface Sampler) ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    # 1. Remove Debug Nodes
    nodes_to_remove = []
    landscape_node = None
    sampler_node = None
    transform_node = None
    
    for node in graph.nodes:
        t = str(node.node_title)
        c = node.get_settings().get_class().get_name()
        
        if "DEBUG" in t:
            nodes_to_remove.append(node)
            
        if "GetLandscape" in c: landscape_node = node
        if "SurfaceSampler" in c: sampler_node = node
        if "TransformPoints" in c: transform_node = node
            
    for n in nodes_to_remove:
        try: 
            print(f"Removing Debug Node: {n.node_title}")
            graph.remove_node(n)
        except: pass
        
    # 2. Configure Landscape Data (Tag)
    if landscape_node:
        try:
            settings = landscape_node.get_settings()
            selector = settings.get_editor_property("actor_selector")
            selector.set_editor_property("actor_selection", unreal.PCGActorSelection.BY_TAG)
            selector.set_editor_property("actor_selection_tag", "DungeonGeneratedLandscape")
            print("Set Landscape Selector: BY TAG (DungeonGeneratedLandscape)")
        except: pass

    # 3. Configure Sampler (Unbounded)
    if sampler_node:
         try:
            s_set = sampler_node.get_settings()
            s_set.set_editor_property("unbounded", True)
            print("Set Sampler: Unbounded = True")
         except: pass

    # 4. Reconnect Chain
    if landscape_node and sampler_node and transform_node:
        # Landscape -> Sampler
        graph.add_edge(landscape_node, "Out", sampler_node, "Surface")
        # Sampler -> Transform
        graph.add_edge(sampler_node, "Out", transform_node, "In")
        print("Reconnected: Landscape -> Sampler -> Transform")
    else:
        print("ERROR: Missing core nodes (Landscape, Sampler, or Transform)")

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

def restore_final_logic():
    print(f"--- [Restore Final Logic] ---", flush=True)
    
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
    restore_final_logic()
