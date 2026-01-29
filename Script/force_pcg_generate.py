import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Force PCG Graph and Generate ===")

world = unreal.EditorLevelLibrary.get_editor_world()

# Load the correct graph
graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
target_graph = unreal.load_asset(graph_path)
print(f"Target Graph Loaded: {target_graph is not None}")

if not target_graph:
    print("ERROR: Cannot load graph!")
else:
    # Find PCG Component
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        comps = actor.get_components_by_class(unreal.PCGComponent)
        for comp in comps:
            print(f"\\nPCG Component: {comp.get_name()}")
            
            # Check current graph_instance
            try:
                current_graph = comp.get_editor_property("graph_instance")
                print(f"Current graph_instance: {current_graph}")
            except Exception as e:
                print(f"graph_instance error: {e}")
            
            # Force set graph using set_graph
            print("Setting Graph...")
            comp.set_graph(target_graph)
            print("Graph Set!")
            
            # Verify
            try:
                new_graph = comp.get_editor_property("graph_instance")
                print(f"New graph_instance: {new_graph}")
            except: pass
            
            # Force Generate
            print("Triggering Generation...")
            comp.generate_local(True)
            print("Generation Triggered!")
            
            # Check if generated
            try:
                output = comp.get_generated_graph_output()
                if output:
                    print(f"Generated Output Exists: True")
                    # Try to get count of generated data
                else:
                    print(f"Generated Output Exists: False (May be async)")
            except Exception as e:
                print(f"Output check error: {e}")

print("\\n=== Done ===")
"""

def force_generate():
    print(f"--- [Force PCG Graph & Generate] ---", flush=True)
    
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
    force_generate()
