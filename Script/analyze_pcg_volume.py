import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Analyze PCG Volume ===")

# Check class availability
vol_class = unreal.PCGVolume
print(f"PCGVolume Class: {vol_class}")

# Create a temp volume to inspect
world = unreal.EditorLevelLibrary.get_editor_world()
vol = world.spawn_actor(vol_class, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))

if vol:
    print(f"Spawned Volume: {vol.get_name()}")
    
    # Check Components
    comps = vol.get_components_by_class(unreal.ActorComponent)
    for c in comps:
        print(f"  - Component: {c.get_name()} ({c.get_class().get_name()})")
        
    # Check Brush Component
    brush = vol.brush_component
    if brush:
        print(f"  - Brush Component Found: {brush.get_name()}")
        # Check Box Properties
        # Usually Volumes use a 'Model' or 'Builder'. 
        # Modifying brush bounds via Python is tricky.
        # But we can check if it has 'BoxExtent' or similar.
        print(f"    - Bounds: {brush.bounds}")
    else:
        print("  - No Brush Component accessor found directly.")
        
    # Clean up
    vol.destroy_actor()
    print("Destroyed temp volume.")
else:
    print("Failed to spawn PCGVolume")

print("\\n=== Done ===")
"""

def analyze_pcg_volume():
    print(f"--- [Analyze PCG Volume] ---", flush=True)
    
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
    analyze_pcg_volume()
