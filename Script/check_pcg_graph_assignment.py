import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Graph Assignment Check ===")

world = unreal.EditorLevelLibrary.get_editor_world()

# Find PCG Component
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    for comp in comps:
        print(f"\\nPCG Component: {comp.get_name()}")
        print(f"Owner: {actor.get_name()}")
        
        # Check graph_instance property
        try:
            graph = comp.get_editor_property("graph")
            print(f"Graph (via property): {graph}")
            if graph:
                print(f"  Graph Name: {graph.get_name()}")
                print(f"  Node Count: {len(graph.nodes)}")
        except Exception as e:
            print(f"Graph property error: {e}")
        
        # Check is_activated
        try:
            activated = comp.get_editor_property("activated")
            print(f"Activated: {activated}")
        except Exception as e:
            print(f"Activated error: {e}")
        
        # Check generation trigger
        try:
            trigger = comp.get_editor_property("generation_trigger")
            print(f"Generation Trigger: {trigger}")
        except Exception as e:
            print(f"Trigger error: {e}")
        
        # List all properties with 'graph' in name
        print("\\nGraph-related properties:")
        for p in dir(comp):
            if 'graph' in p.lower():
                print(f"  - {p}")

# Also check DungeonTheme
print("\\n=== Theme Check ===")
builders = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/DungeonGenerator.DungeonWorldBuilder"))
if builders:
    builder = builders[0]
    try:
        theme = builder.get_editor_property("dungeon_theme")
        if theme:
            print(f"Theme: {theme.get_name()}")
            try:
                nature_graph = theme.get_editor_property("nature_pcg_graph")
                print(f"NaturePCGGraph: {nature_graph}")
                if nature_graph:
                    print(f"  Graph Name: {nature_graph.get_name()}")
            except Exception as e:
                print(f"NaturePCGGraph error: {e}")
        else:
            print("Theme: *** NULL *** (CRITICAL)")
    except Exception as e:
        print(f"Theme error: {e}")
"""

def check_graph():
    print(f"--- [PCG Graph Assignment Check] ---", flush=True)
    
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
    check_graph()
