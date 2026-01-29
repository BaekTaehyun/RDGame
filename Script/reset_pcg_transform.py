import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Reset PCG Transform (Global Anchor) ===")

world = unreal.EditorLevelLibrary.get_editor_world()

pcg_actor = None
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    if comps:
        pcg_actor = actor
        break

if pcg_actor:
    print(f"Target PCG Actor: {pcg_actor.get_name()}")
    
    # Current
    loc = pcg_actor.get_actor_location()
    print(f"  Old Location: {loc}")
    
    # Reset to 0,0,0
    new_loc = unreal.Vector(0, 0, 0)
    pcg_actor.set_actor_location(new_loc, False, True)
    
    # Reset Rotation
    pcg_actor.set_actor_rotation(unreal.Rotator(0, 0, 0), True)
    
    # Set Huge Scale (Global Coverage)
    pcg_actor.set_actor_scale3d(unreal.Vector(100, 100, 100))
    
    print("  New Location: (0, 0, 0)")
    print("  New Scale: (100, 100, 100)")
    
    # Regen
    comps = pcg_actor.get_components_by_class(unreal.PCGComponent)
    if comps:
        comps[0].generate_local(True)
        print("Regenerated.")
else:
    print("PCG Actor not found.")

print("\\n=== Done ===")
"""

def reset_pcg_transform():
    print(f"--- [Reset PCG Transform] ---", flush=True)
    
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
    reset_pcg_transform()
