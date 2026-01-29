import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Reduce Bounds (V2) ---")

graph = unreal.load_asset(graph_path)
if graph:
    bounds_node = None
    lift_node = None
    
    # Iterate safely
    for n in graph.nodes:
        nm = n.get_name()
        
        # 1. Bounds Modifier
        if "BoundsModifier" in nm:
            bounds_node = n
            
        # 2. Lift Node (TransformPoints)
        if "TransformPoints" in nm:
            # Check if this is the generic transform or the lift one
            # Lift one has high Z offset.
            try:
                # settings = n.get_settings() # This returns PCGSettings object
                # To read property, we access settings object directly?
                # Actually 'n.get_settings()' returns the settings OBJECT.
                # get_editor_property works on that object.
                s = n.get_settings()
                
                # Check OffsetMin
                off = s.get_editor_property("OffsetMin")
                if off.z > 500: # Previously 5000 or 10000
                    lift_node = n
            except: pass

    # Apply Fixes
    if bounds_node:
        try:
            sz = 240.0
            s = bounds_node.get_settings()
            s.set_editor_property("BoundsMin", unreal.Vector(-sz, -sz, -sz))
            s.set_editor_property("BoundsMax", unreal.Vector(sz, sz, sz))
            print("BoundsModifier -> +/- 240")
        except Exception as e:
            print(f"Bounds Error: {e}")

    if lift_node:
        try:
            # Reduce to 2000
            lift_z = unreal.Vector(0, 0, 2000)
            s = lift_node.get_settings()
            s.set_editor_property("OffsetMin", lift_z)
            s.set_editor_property("OffsetMax", lift_z)
            print("Lift -> Z+2000")
        except Exception as e:
            print(f"Lift Error: {e}")
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)

# Force Gen
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == "PCGNature":
        target_actor = a
        break
        
if target_actor:
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        try:
            pcg_comp.generate(True)
            print("Generated.")
        except: pass

"""

def fix_bounds_v2():
    print(f"--- [Fix] Fix Bounds V2 ---", flush=True)
    
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
    fix_bounds_v2()
