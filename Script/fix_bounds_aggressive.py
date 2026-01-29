import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Aggressive Bounds Expansion ---")

# 1. Expand Graph Internal Bounds (BoundsModifier)
graph = unreal.load_asset(graph_path)
if graph:
    bounds_node = None
    for n in graph.nodes:
        if "BoundsModifier" in n.get_name():
            bounds_node = n
            break
            
    if bounds_node:
        try:
            # 5000 = 50 meters radius around each point.
            # If points are sparse, this ensures they overlap.
            sz = 5000.0
            bounds_node.get_settings().set_editor_property("BoundsMin", unreal.Vector(-sz, -sz, -sz))
            bounds_node.get_settings().set_editor_property("BoundsMax", unreal.Vector(sz, sz, sz))
            print("Graph BoundsModifier -> +/- 5000")
        except: pass
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)

# 2. Check Actor Bounds
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == "PCGNature":
        target_actor = a
        break
        
if target_actor:
    bounds = target_actor.get_actor_bounds(False) # bOnlyCollidingComponents=False
    print(f"Actor Origin: {bounds[0]}")
    print(f"Actor BoxExtent: {bounds[1]}")
    
    # If Extent is small, generation is clipped.
    # We can't set bounds directly (readonly).
    # We must Scale or use Unbound property.
    # Since 'bIsUnbound' failed on Component, let's try finding the PROPERTY name via iteration.
    
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        # PCGComponent properties inspection?
        # Let's try to set 'GenerationBounds'.
        # Or Just FORCE GENERATE again.
        
        # NOTE: If user says "Position is different", maybe Bounds CENTER is wrong?
        # Bounds Origin = Actor Location.
        # If Actor Location = (0,0,0), then Bounds Center = (0,0,0).
        # BoxExtent = (50,50,50) -> Covers -50 to +50.
        # Dungeon is at (2000, 2000). It is OUTSIDE.
        
        # We need Bounds to cover the Dungeon.
        # Set Actor Scale 10000 -> Extent 500,000. Covers everything.
        # User said "No" to that? Or maybe I undid it?
        # Step 6445: I reset Scale to 1.
        # SO: Bounds became small again (50?).
        # Dungeon is outside.
        
        # FIX: Move Actor to Center of Dungeon?
        # OR Scale Actor Up again?
        
        # User said "Not Coordinate, Bounds".
        # So he implies "Make the Box Bigger so it covers the area".
        
        # Action: Scale Actor to 10000 again.
        target_actor.set_actor_scale3d(unreal.Vector(10000, 10000, 10000))
        print("Re-Applied Scale 10000 (To Cover Dungeon Area)")
        
        pcg_comp.generate(True)
        print("Generated.")

"""

def fix_bounds_aggro():
    print(f"--- [Fix] Aggressive Bounds ---", flush=True)
    
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
    fix_bounds_aggro()
