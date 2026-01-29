import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Inspect] PCG Component Bounds ---")
world = unreal.EditorLevelLibrary.get_editor_world()

# Find Dungeon Actor
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/DungeonGenerator.DungeonWorldBuilder"))
if not actors:
    print("No DungeonWorldBuilder found.")
else:
    dungeon = actors[0]
    print(f"Dungeon Actor: {dungeon.get_name()}")
    print(f"Location: {dungeon.get_actor_location()}")
    
    # Find PCG Component
    pcg_comps = dungeon.get_components_by_class(unreal.PCGComponent)
    print(f"PCG Components: {len(pcg_comps)}")
    
    for comp in pcg_comps:
        bounds = comp.get_local_bounds() # min, max
        origin = comp.get_length() # wait, calc bounds
        
        # Get World Bounds
        origin, extent = comp.get_actor_bounds() 
        # Wait, component bounds
        
        # Use get_bounds()
        origin, box_extent, sphere_radius = comp.get_bounds()
        print(f"Comp: {comp.get_name()}")
        print(f"  Origin: {origin}")
        print(f"  BoxExtent: {box_extent}")
        print(f"  Graph: {comp.get_graph().get_name() if comp.get_graph() else 'None'}")
        
        # Check if it has generated data
        # Not easily exposed, but we can check if it is managed
"""

def inspect_bounds():
    print(f"--- [Inspect] PCG Component ---", flush=True)
    
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
    inspect_bounds()
