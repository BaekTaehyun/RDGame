import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Fix] PCG Volume Bounds ---")

# 1. Find the PCG Volume Actor
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_sub.get_all_level_actors()

target_actor = None
for a in actors:
    # Check Name or Label
    if "PCG_Nature_Wood" in a.get_actor_label():
        target_actor = a
        break
        
if target_actor:
    print(f"Found Actor: {target_actor.get_actor_label()}")
    
    # 2. Fix Bounds
    # PCGVolume usually has a PCGComponent and a Brush/BoxComponent.
    
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        try:
            # Option A: Set Unbound (Infinite)
            # Property: bIsUnbound? Check PCGProcessGraphSettings?
            # It's usually on the Volume itself if it inherits from PCGVolume.
            # actually PCGVolume has 'bIsUnbound'.
            target_actor.set_editor_property("bIsUnbound", True)
            print("Set bIsUnbound = True")
        except:
             print("Could not set bIsUnbound on Actor (Might not be PCGVolume class)")

    # Option B: Fix Brush Bounds (if not unbound)
    # Even if unbound, 'Invalid Bounds' might come from the Component check.
    
    brush_comp = target_actor.get_component_by_class(unreal.BrushComponent)
    if brush_comp:
        try:
            # Set big bounds.
            # Accessing Brush parameters is tricky via Python.
            # Trigger 'UpdateBounds'.
            pass
        except: pass
        
    # Scale Actor to ensure it has volume?
    # Infinite bounds should handle it, but allow for valid scale.
    current_scale = target_actor.get_actor_scale3d()
    if current_scale.x < 1.0:
        target_actor.set_actor_scale3d(unreal.Vector(100, 100, 100))
        print("Scaled up Actor.")

    # 3. Force Generate
    if pcg_comp:
        # Toggle Generate
        pcg_comp.generate()
        print("Triggered Generate.")
        
else:
    print("PCG Volume Actor NOT FOUND in Level.")

"""

def fix_volume_bounds():
    print(f"--- [Fix] Fixing Volume Bounds ---", flush=True)
    
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
    fix_volume_bounds()
