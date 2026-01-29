import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Graph API Check & Landscape Refresh ===")

# 1. Check Graph Methods
graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if graph:
    print(f"Graph Loaded: {graph.get_name()}")
    print("Methods available on PCGGraph:")
    valid_methods = []
    for m in dir(graph):
        if not m.startswith('_'):
            valid_methods.append(m)
    print(valid_methods)

# 2. Refresh Landscape
print("\\n=== Refreshing Landscape ===")
found = False
for actor in unreal.GameplayStatics.get_all_actors_of_class(unreal.EditorLevelLibrary.get_editor_world(), unreal.Landscape):
    if "DungeonGeneratedLandscape" in [str(t) for t in actor.tags]:
        print(f"Found Target Landscape: {actor.get_name()}")
        # Force update
        actor.modify()
        actor.post_edit_change()
        print("Called post_edit_change()")
        found = True

if not found:
    print("Target Landscape NOT found!")

# 3. Retry Regen
print("\\n=== Regenerating PCG ===")
# Find PCG Component
for actor in unreal.GameplayStatics.get_all_actors_of_class(unreal.EditorLevelLibrary.get_editor_world(), unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    if comps:
        comp = comps[0]
        comp.generate_local(True)
        print(f"Regenerated {comp.get_name()}")
        
        # Check Output
        try:
            data = comp.get_generated_graph_output()
            if data:
                # Count points
                total_points = 0
                if hasattr(data, 'tagged_data'):
                    for td in data.tagged_data:
                        if hasattr(td.data, 'get_points'):
                            total_points += len(td.data.get_points())
                print(f"Total Generated Points: {total_points}")
        except: pass

print("\\n=== Done ===")
"""

def debug_api_and_refresh():
    print(f"--- [API Check & Refresh] ---", flush=True)
    
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
    debug_api_and_refresh()
