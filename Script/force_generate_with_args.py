import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Fix] Force Generate (Scaled) ---")

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == "PCGNature":
        target_actor = a
        break
        
if target_actor:
    print(f"Target: {target_actor.get_actor_label()}")
    
    # Scale Up to ensure Bounds cover everything (since bIsUnbound failed)
    target_actor.set_actor_scale3d(unreal.Vector(10000, 10000, 10000))
    print("Scaled to 10000.")
    
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        try:
            pcg_comp.generate(True) # Force = True
            print("Force Generated.")
        except Exception as e:
            print(f"Generate Error: {e}")
"""

def force_gen():
    print(f"--- [Fix] Force Gen ---", flush=True)
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
        req_id = int(time.time()*1000)
        req = {"jsonrpc": "2.0", "method": method, "params": params}
        if expect_response: req["id"] = req_id
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
    force_gen()
