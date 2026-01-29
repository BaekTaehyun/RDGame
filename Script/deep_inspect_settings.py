import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Deep Inspect Landscape & Sampler ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        cname = settings.get_class().get_name()
        
        if "GetLandscape" in cname:
            print(f"\\n[GetLandscapeData]")
            # Inspect Selector
            try:
                sel = settings.get_editor_property("actor_selector")
                mode = sel.get_editor_property("actor_selection") # Enum
                tag = sel.get_editor_property("actor_selection_tag") # Name
                must_overlap = sel.get_editor_property("must_overlap_self") # Bool
                
                print(f"  Selection Mode: {mode}")
                print(f"  Selection Tag: '{tag}'")
                print(f"  Must Overlap: {must_overlap}")
                
            except Exception as e:
                print(f"  Selector Read Error: {e}")

        if "SurfaceSampler" in cname:
            print(f"\\n[SurfaceSampler]")
            try:
                unbounded = settings.get_editor_property("unbounded")
                looseness = settings.get_editor_property("looseness")
                try:
                    ppsm = settings.get_editor_property("points_per_square_meter")
                except:
                    # Properties might differ by version
                    ppsm = "N/A (Property not found)"
                
                print(f"  Unbounded: {unbounded}")
                print(f"  Looseness: {looseness}")
                print(f"  Points/m2: {ppsm}")
            except Exception as e:
                 print(f"  Sampler Read Error: {e}")

    # Check Locations
    world = unreal.EditorLevelLibrary.get_editor_world()
    
    # 1. PCG Component Location
    pcg_loc = None
    pcg_bounds = None
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        comps = actor.get_components_by_class(unreal.PCGComponent)
        if comps:
            pcg_loc = actor.get_actor_location()
            pcg_bounds = actor.get_actor_bounds(False) # Origin, Extent
            print(f"\\nPCG Component Owner: {actor.get_name()}")
            print(f"  Location: {pcg_loc}")
            print(f"  Bounds Origin: {pcg_bounds[0]}")
            print(f"  Bounds Extent: {pcg_bounds[1]}")
            break
            
    # 2. Landscape Location
    land_loc = None
    land_bounds = None
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape):
        if "DungeonGeneratedLandscape" in [str(t) for t in actor.tags]:
            land_loc = actor.get_actor_location()
            land_bounds = actor.get_actor_bounds(False)
            print(f"\\nLandscape: {actor.get_name()}")
            print(f"  Location: {land_loc}")
            print(f"  Bounds Origin: {land_bounds[0]}")
            print(f"  Bounds Extent: {land_bounds[1]}")
            break

print("\\n=== Done ===")
"""

def deep_inspect_settings():
    print(f"--- [Deep Inspect Settings] ---", flush=True)
    
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
    deep_inspect_settings()
