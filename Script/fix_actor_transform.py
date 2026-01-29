import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Fix] Reset Actor Transform & Bounds ---")

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == "PCGNature":
        target_actor = a
        break
        
if target_actor:
    print(f"Target: {target_actor.get_actor_label()}")
    
    # 1. Reset Transform (Align with World Origin)
    target_actor.set_actor_location(unreal.Vector(0, 0, 0), False, False)
    target_actor.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
    target_actor.set_actor_scale3d(unreal.Vector(1, 1, 1))
    print("Reset Transform to Identity (0,0,0 | 1,1,1).")
    
    # 2. Expand Brush Bounds (Without Scaling Actor)
    brush_comp = target_actor.get_component_by_class(unreal.BrushComponent)
    if brush_comp:
        try:
            # Try to set BoxExtent (if it's a Box Brush/Shape)
            # Or use 'Bounds' property?
            # BrushComponent usually has 'Brush' object. 
            # If it's a Volume, the BrushComponent holds the geometry.
            # Editing Brush geometry via Python is hard.
            # However, standard Volumes allow changing 'BrushBuilder' params if newly creating.
            # Existing Volume? maybe just scale the *Component*?
            # Setting Component Relative Scale?
            
            # Let's try setting Component Scale to (10000, 10000, 10000) while Actor is (1,1,1).
            # This expands the bounds.
            brush_comp.set_relative_scale3d(unreal.Vector(50000, 50000, 50000))
            print("Expanded Brush Component Scale.")
        except Exception as e:
            print(f"Brush Error: {e}")
    else:
        print("No BrushComponent found (Not a Volume?)")

    # 3. Force Generate
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        try:
            pcg_comp.generate(True)
            print("Force Generated.")
        except Exception as e:
            print(f"Gen Error: {e}")

"""

def fix_transform():
    print(f"--- [Fix] Actor Transform ---", flush=True)
    
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
    fix_transform()
