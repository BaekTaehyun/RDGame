import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Force Regen Anchor ===")
world = unreal.EditorLevelLibrary.get_editor_world()
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if "Anchor" in actor.get_actor_label():
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            comps[0].generate_local(True)
            print(f"Regenerated {actor.get_name()}")
print("=== Done ===")
"""

def force_regen_anchor():
    print(f"--- [Force Regen Anchor] ---", flush=True)
    
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
    force_regen_anchor()
