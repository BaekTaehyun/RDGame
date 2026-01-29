import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== MCP: Inspect PCG Volume ===")

world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)

found = False
for a in actors:
    # Look for the PCG volume we spawned
    if "PCG" in a.get_name() and "Volume" in a.get_class().get_name():
        found = True
        print(f"Found Actor: {a.get_name()}")
        print(f"  - Class: {a.get_class().get_name()}")
        print(f"  - Label: {a.get_actor_label()}")
        print(f"  - Location: {a.get_actor_location()}")
        
        # Check Components
        comps = a.get_components_by_class(unreal.ActorComponent)
        print(f"  - Components ({len(comps)}):")
        for c in comps:
            print(f"    * {c.get_name()} ({c.get_class().get_name()})")
            
        # Check Brush/Bounds
        bounds = a.get_actor_bounds(False) # origin, extent
        print(f"  - Bounds Origin: {bounds[0]}")
        print(f"  - Bounds BoxExtent: {bounds[1]}")
        
        # Calculate Size
        size = unreal.Vector(bounds[1].x * 2, bounds[1].y * 2, bounds[1].z * 2)
        print(f"  - Calculated Size: {size}")
        
        print("-" * 30)

if not found:
    print("WARNING: No PCGVolume found in scene.")

print("=== Done ===")
"""

def inspect_pcg_volume():
    print(f"--- [Inspect PCG Volume] ---", flush=True)
    
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
    inspect_pcg_volume()
