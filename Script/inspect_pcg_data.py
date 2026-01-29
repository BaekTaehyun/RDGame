import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Generated Data Inspection ===")

world = unreal.EditorLevelLibrary.get_editor_world()

# Find PCG Component
pcg_comp = None
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    if comps:
        pcg_comp = comps[0]
        print(f"Found PCG Component on {actor.get_name()}")
        break

if not pcg_comp:
    print("ERROR: No PCG Component found")
else:
    # Try to access generated data
    # Note: access method depends on UE version.
    
    # Method 1: get_generated_graph_output
    try:
        data = pcg_comp.get_generated_graph_output()
        if data:
            print(f"Generated Output Found: {data}")
            
            # Usually data is a UPCGData object or collection
            # Let's try to inspect it
            if hasattr(data, 'tagged_data'):
                print(f"Tagged Data Count: {len(data.tagged_data)}")
                for i, tagged in enumerate(data.tagged_data):
                    print(f"  [{i}] Pin: {tagged.pin}")
                    d = tagged.data
                    print(f"      Data Type: {d.get_class().get_name()}")
                    
                    if "PCGPointData" in d.get_class().get_name():
                        points = d.get_points()
                        print(f"      Point Count: {len(points)}")
                        if len(points) > 0:
                            p = points[0]
                            print(f"      Sample Point 0 Loc: {p.transform.translation}")
                            print(f"      Sample Point 0 Scale: {p.transform.scale3d}")
            else:
                print("Data does not have tagged_data")
        else:
            print("get_generated_graph_output() returned None")
    except Exception as e:
        print(f"Error accessing generated output: {e}")

print("\\n=== Done ===")
"""

def inspect_data():
    print(f"--- [Inspect PCG Data] ---", flush=True)
    
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
    inspect_data()
