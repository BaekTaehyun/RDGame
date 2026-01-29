import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Align PCG to Dungeon & Remove Lift ---")

# 1. Remove Lift Node (Cleanup)
graph = unreal.load_asset(graph_path)
if graph:
    lift_node = None
    proj_node = None
    grid_node = None
    
    for n in graph.nodes:
        # Identify Lift by checking offset or just remove the one we added last
        # We added it at (650, -200) or (600, -200).
        # Or just remove ANY Transform that has (0,0,5000) offset.
        if "TransformPoints" in n.get_name():
            # Check settings
            try:
                settings = n.get_settings()
                min_off = settings.get_editor_property("OffsetMin")
                if min_off.z > 4000: # Found it
                    lift_node = n
            except: pass
            
        if "Projection" in n.get_name(): proj_node = n
        if "CreatePointsGrid" in n.get_name(): grid_node = n
        
    if lift_node:
        try:
            # We need to bridge connections before deleting
            # Grid -> Lift -> Proj  ==>  Grid -> Proj
            if grid_node and proj_node:
                graph.add_edge(grid_node, "Out", proj_node, "In")
            
            graph.remove_node(lift_node)
            print("Removed Lift Node.")
        except: pass
    
    unreal.EditorAssetLibrary.save_loaded_asset(graph)

# 2. Align Actor
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
pcg_actor = None
dungeon_actor = None

for a in actor_sub.get_all_level_actors():
    label = a.get_actor_label()
    if label == "PCGNature": pcg_actor = a
    # Heuristic for Dungeon Actor
    if "Dungeon" in label and "Generator" in label: dataset_actor = a # E.g. BP_DungeonGenerator
    elif "DungeonFullTestActor" in label: dungeon_actor = a
    elif "Dungeon" in label and not pcg_actor: dungeon_actor = a
    
if pcg_actor and dungeon_actor:
    loc = dungeon_actor.get_actor_location()
    print(f"Aligning PCGNature to {dungeon_actor.get_actor_label()} at {loc}")
    
    pcg_actor.set_actor_location(loc, False, False)
    pcg_actor.set_actor_rotation(dungeon_actor.get_actor_rotation(), False)
    pcg_actor.set_actor_scale3d(unreal.Vector(1, 1, 1)) # Reset scale
    
    # Force Generate
    pcg_comp = pcg_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        pcg_comp.generate(True)
        print("Aligned & Generated.")
else:
    print(f"Actors Missed - PCG: {pcg_actor}, Dungeon: {dungeon_actor}")

"""

def align_pcg():
    print(f"--- [Fix] Align PCG ---", flush=True)
    
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
    align_pcg()
