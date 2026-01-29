import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Force Scale & Inspect Density ===")

# 1. Scale Up PCG Actor
world = unreal.EditorLevelLibrary.get_editor_world()
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    if comps:
        print(f"Scaling Up {actor.get_name()}...")
        # Scale to 10000 (Huge coverage)
        actor.set_actor_scale3d(unreal.Vector(100.0, 100.0, 100.0))
        print("Set Scale to (100, 100, 100)")
        break

# 2. Find Density Property Name
graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if graph:
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        cname = settings.get_class().get_name()
        
        if "SurfaceSampler" in cname:
            print(f"\\nProperties of {cname}:")
            # List all floats
            for p in dir(settings):
                # Try simple heuristc
                if 'points' in p.lower() or 'density' in p.lower():
                     try:
                         val = getattr(settings, p)
                         if not callable(val):
                             print(f"  {p}: {val}")
                     except: pass
            
            # Try to start setting it if we find "point_extents" or similar
            # Known names: 'points_per_squared_meter', 'point_extents', 'looseness'
            
            # Force set if we find a good candidate
            try:
                # Some versions use 'points_per_squared_meter' (squared with d)
                 settings.set_editor_property("points_per_squared_meter", 0.1)
                 print("  Set points_per_squared_meter = 0.1")
            except: pass

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    
    # Regen
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            comps[0].generate_local(True)
            print("Regenerated.")

print("\\n=== Done ===")
"""

def force_scale_and_density():
    print(f"--- [Force Scale & Density] ---", flush=True)
    
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
    force_scale_and_density()
