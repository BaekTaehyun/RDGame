import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Reduce Bounds & Verify Logic ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Reduce BoundsModifier
    bounds_node = None
    lift_node = None
    
    for n in graph.nodes:
        if "BoundsModifier" in n.get_name(): bounds_node = n
        if "Lift" in n.get_editor_property("NodeTitleOverride") or "TransformPoints" in n.get_name():
            # Find the component with Z offset > 1000
            try:
                if n.get_settings().get_editor_property("OffsetMin").z > 1000:
                    lift_node = n
            except: pass

    if bounds_node:
        try:
            # Set to 240 (Tile Size approx)
            sz = 240.0
            bounds_node.get_settings().set_editor_property("BoundsMin", unreal.Vector(-sz, -sz, -sz))
            bounds_node.get_settings().set_editor_property("BoundsMax", unreal.Vector(sz, sz, sz))
            print("BoundsModifier -> +/- 240 (Tile Size)")
        except: pass
        
    # 2. Adjust Lift (Reduce to 2000 to be less insane)
    if lift_node:
        try:
            lift_z = unreal.Vector(0, 0, 2000)
            lift_node.get_settings().set_editor_property("OffsetMin", lift_z)
            lift_node.get_settings().set_editor_property("OffsetMax", lift_z)
            print("Lift -> Z+2000")
        except: pass

    # 3. Check DungeonDataReader
    # We want to ensure it is connected and has correct properties
    # Just print them for now.
    
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Logic Adjusted.")

# 4. Force Generate on PCGNature
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == "PCGNature":
        target_actor = a
        break

if target_actor:
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        pcg_comp.generate(True)
        print("Generated.")

"""

def fix_bounds_logic():
    print(f"--- [Fix] Fix Bounds Logic ---", flush=True)
    
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
    fix_bounds_logic()
