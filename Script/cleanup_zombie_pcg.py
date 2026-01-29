import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Cleanup Zombie PCG Actor ===")

world = unreal.EditorLevelLibrary.get_editor_world()

deleted_count = 0

for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    # Identify the OLD one. 
    # The NEW one is 'PCG_Nature_Anchor'. 
    # The OLD one is likely 'PCGNature' or 'PCGNature_C...'.
    # We want to keep 'Anchor'.
    
    name = actor.get_name()
    label = actor.get_actor_label()
    
    if "PCG" in label or "Nature" in label:
        if "Anchor" not in label:
            print(f"Deleting Zombie Actor: {name} ({label}) at {actor.get_actor_location()}")
            unreal.EditorLevelLibrary.destroy_actor(actor)
            deleted_count += 1
        else:
            print(f"Keeping Anchor: {name} ({label})")

print(f"Deleted {deleted_count} actors.")

# Force Regen on the remaining Anchor
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if "Anchor" in actor.get_actor_label():
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            print(f"Regenerating {actor.get_actor_label()}...")
            comps[0].generate_local(True)

print("\\n=== Done ===")
"""

def cleanup_zombie_pcg():
    print(f"--- [Cleanup Zombie PCG] ---", flush=True)
    
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
    cleanup_zombie_pcg()
