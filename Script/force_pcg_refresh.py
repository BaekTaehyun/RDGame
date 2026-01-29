import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Sync] Forcing PCG Refresh ---")

# 1. Find all PCG Components in the Level
# We want to refresh the one using "PCG_Nature_Wood"
target_graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
refreshed_count = 0

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for actor in actors:
    # Check components
    # Using getattr to be safe if class doesn't have it
    comps = actor.get_components_by_class(unreal.PCGComponent)
    
    for comp in comps:
        # Check if it uses our graph
        # Prop: Graph
        used_graph = comp.get_editor_property("Graph")
        if used_graph and used_graph.get_path_name() == target_graph_path:
            print(f"Refreshing Component on {actor.get_name()}...")
            
            # API might be 'generate_local' or 'generate'
            # Let's try likely candidates
            try:
                # Force cleanup first to ensure visual update
                comp.cleanup_local()
                
                # Generate
                comp.generate_local(True) # True = Force?
                print("  -> Regenerated.")
                refreshed_count += 1
            except Exception as e:
                print(f"  -> Refresh Error: {e}")
                # Fallback: Dirty the component
                comp.set_component_tick_enabled(False)
                comp.set_component_tick_enabled(True)

if refreshed_count == 0:
    print("No matching PCG Components found in level to refresh.")
else:
    print(f"Refreshed {refreshed_count} PCG Components.")

"""

def force_refresh():
    print(f"--- [Sync] Refreshing ---", flush=True)
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
        req_id = int(time.time()*1000)
        req = {"jsonrpc": "2.0", "method": method, "params": params}
        if expect_response: req["id"] = req_id
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
    force_refresh()
