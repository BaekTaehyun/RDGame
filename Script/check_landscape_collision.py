import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Check Landscape Collision ===")

world = unreal.EditorLevelLibrary.get_editor_world()
landscapes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)

if not landscapes:
    print("ERROR: No Landscape Found!")
else:
    land = landscapes[0]
    print(f"Landscape: {land.get_name()}")
    
    # Check Components
    comps = land.get_components_by_class(unreal.LandscapeComponent)
    print(f"Components: {len(comps)}")
    
    if comps:
        c = comps[0]
        print(f"  - Collision Enabled: {c.get_collision_enabled_type_name(c.get_collision_enabled())}")
        print(f"  - Profile: {c.get_collision_profile_name()}")
        print(f"  - Response to WorldStatic: {c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_WORLD_STATIC)}")
        
    # Check Actor Enable Collision
    print(f"  - Actor Enable Collision: {land.get_actor_enable_collision()}")

print("\\n=== Done ===")
"""

def check_landscape_collision():
    print(f"--- [Check Landscape Collision] ---", flush=True)
    
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
    check_landscape_collision()
