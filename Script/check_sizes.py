import sys
import json
import subprocess

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Check Dungeon Grid and Landscape Size ===")

world = unreal.EditorLevelLibrary.get_editor_world()

# Find Landscape
landscapes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)
for ls in landscapes:
    origin, extent = ls.get_actor_bounds(False)
    print(f"Landscape: {ls.get_actor_label()}")
    print(f"  - Origin: {origin}")
    print(f"  - Extent: {extent}")
    print(f"  - Size: {extent.x * 2} x {extent.y * 2} x {extent.z * 2}")

# Find PCG Volumes
volumes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PCGVolume)
for vol in volumes:
    origin, extent = vol.get_actor_bounds(False)
    scale = vol.get_actor_scale3d()
    print(f"\\nPCGVolume: {vol.get_actor_label()}")
    print(f"  - Scale: {scale}")
    print(f"  - Extent: {extent}")
    print(f"  - Size: {extent.x * 2} x {extent.y * 2} x {extent.z * 2}")

print("\\n=== Done ===")
"""

def check_sizes():
    print(f"--- [Check Grid/Landscape Sizes] ---", flush=True)
    
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
    check_sizes()
