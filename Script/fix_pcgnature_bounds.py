import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Fix] Fixing 'PCGNature' Bounds ---")

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == "PCGNature":
        target_actor = a
        break
        
if target_actor:
    print(f"Found Target: {target_actor.get_actor_label()}")
    
    # Check Components
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        try:
            # 1. Set Unbound on Component (If property exists)
            # 2. Set Unbound on Actor (If PCGVolume)
            # 'PCGNature_C' is likely a Blueprint inheriting from Actor or PCGVolume.
            
            # Try setting on Component directly?
            # PCGComponent has 'bIsUnbound'? Usually it matches the Volume.
            # Let's try to find property on Comp first.
            try:
                pcg_comp.set_editor_property("bIsUnbound", True)
                print("Set PCGComponent.bIsUnbound = True")
            except:
                print("PCGComponent has no 'bIsUnbound'. Checking Actor...")
                
            # Try on Actor
            try:
                target_actor.set_editor_property("bIsUnbound", True)
                print("Set Actor.bIsUnbound = True")
            except:
                 print("Actor has no 'bIsUnbound'.")

            # 3. Increase Bounds on Component (Bounds scale?)
            # Usually handled by Volume.
            
            # 4. Trigger Generation
            pcg_comp.generate()
            print("Triggered Generation.")
            
        except Exception as e:
            print(f"Error accessing Component: {e}")
    else:
        print("Actor has no PCGComponent.")
else:
    print("'PCGNature' Not Found.")

"""

def fix_pcgnature():
    print(f"--- [Fix] PCGNature Bounds ---", flush=True)
    
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
    fix_pcgnature()
