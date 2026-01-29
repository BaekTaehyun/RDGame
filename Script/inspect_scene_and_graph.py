import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Scene & Graph Inspection ===")

world = unreal.EditorLevelLibrary.get_editor_world()

# 1. List PCG Actors
print("--- [PCG Actors] ---")
pcg_actors = []
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if "PCG" in actor.get_name() or "Nature" in actor.get_name():
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            print(f"Found PCG Actor: '{actor.get_name()}'")
            print(f"  - Label: {actor.get_actor_label()}")
            print(f"  - Location: {actor.get_actor_location()}")
            pcg_actors.append(actor)
            
# 2. List Landscape Actors
print("\\n--- [Landscape Actors] ---")
landscapes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)
for l in landscapes:
    print(f"Found Landscape: '{l.get_name()}'")
    print(f"  - Label: {l.get_actor_label()}")
    print(f"  - Location: {l.get_actor_location()}")
    print(f"  - Bounds: {l.get_actor_bounds(False)}")
    print(f"  - Tags: {l.tags}")

# 3. Check Graph Settings ('Get Landscape Data')
print("\\n--- [Graph Settings] ---")
graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)
if graph:
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        
        if "GetLandscape" in settings.get_class().get_name():
            print(f"Node: {node.get_name()} ({node.node_title})")
            
            try:
                # Check Selector
                selector = settings.get_editor_property("actor_selector")
                mode = selector.get_editor_property("actor_selection") # Enum
                cls = selector.get_editor_property("actor_selection_class")
                tag = selector.get_editor_property("actor_selection_tag")
                
                print(f"  - Mode: {mode} (0=Tag, 1=Name, 2=Class)")
                print(f"  - Class: {cls}")
                print(f"  - Tag: {tag}")
            except Exception as e:
                print(f"  - Error reading selector: {e}")
                
            # Check Unbounded
            try:
                unbounded = settings.get_editor_property("unbounded")
                print(f"  - Unbounded: {unbounded}")
            except: pass


print("\\n=== Done ===")
"""

def inspect_scene_and_graph():
    print(f"--- [Inspect Scene & Graph] ---", flush=True)
    
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
    inspect_scene_and_graph()
