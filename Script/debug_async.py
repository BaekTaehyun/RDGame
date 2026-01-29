import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal
import time

print("=== PCG Async Check ===")

world = unreal.EditorLevelLibrary.get_editor_world()

for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    if comps:
        comp = comps[0]
        print(f"Triggering Generation for {comp.get_name()}")
        comp.generate_local(True)
        
        # Check immediately
        print("Checking immediately...")
        if comp.is_generating():
             print("  Status: Generating...")
        else:
             print("  Status: Idle (Finished or Not Started)")
             
        # Wait loop
        for i in range(5):
            time.sleep(1.0)
            print(f"Waited {i+1}s...")
            if not comp.is_generating():
                print("Generation Finished!")
                break
        
        # Check Output
        try:
            data = comp.get_generated_graph_output()
            if data:
                print(f"Tagged Data Entries: {len(data.tagged_data)}")
                total = 0
                for td in data.tagged_data:
                    if hasattr(td.data, 'get_points'):
                        pts = td.data.get_points()
                        total += len(pts)
                print(f"Total Points: {total}")
        except Exception as e:
            print(f"Error reading output: {e}")

print("\\n=== Done ===")
"""

def debug_async():
    print(f"--- [Debug Async] ---", flush=True)
    
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
    debug_async()
