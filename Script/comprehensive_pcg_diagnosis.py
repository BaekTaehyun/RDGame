import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("="*60)
print("       COMPREHENSIVE PCG DIAGNOSIS")
print("="*60)

world = unreal.EditorLevelLibrary.get_editor_world()

# === 1. LANDSCAPE CHECK ===
print("\\n[1] LANDSCAPE STATUS")
print("-"*40)

landscapes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)
print(f"Total Landscapes: {len(landscapes)}")

tagged_landscapes = unreal.GameplayStatics.get_all_actors_with_tag(world, "DungeonGeneratedLandscape")
print(f"Tagged Landscapes: {len(tagged_landscapes)}")

if landscapes:
    land = landscapes[0]
    comps = land.get_components_by_class(unreal.LandscapeComponent)
    if comps:
        print(f"Collision Profile: {comps[0].get_collision_profile_name()}")
    print(f"Actor Tags: {land.tags}")

# === 2. DUNGEON WORLD BUILDER CHECK ===
print("\\n[2] DUNGEON WORLD BUILDER STATUS")
print("-"*40)

builders = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/DungeonGenerator.DungeonWorldBuilder"))
if not builders:
    print("ERROR: No DungeonWorldBuilder found!")
else:
    builder = builders[0]
    print(f"Builder: {builder.get_name()}")
    print(f"Location: {builder.get_actor_location()}")

# === 3. PCG COMPONENT CHECK ===
print("\\n[3] PCG COMPONENT STATUS")
print("-"*40)

pcg_comps = []
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    for c in comps:
        pcg_comps.append((actor, c))

print(f"Total PCG Components: {len(pcg_comps)}")

for actor, comp in pcg_comps:
    print(f"\\n  Owner: {actor.get_name()}")
    print(f"  Comp Name: {comp.get_name()}")
    
    # Check Graph
    graph = comp.get_graph()
    if graph:
        print(f"  Graph: {graph.get_name()}")
    else:
        print(f"  Graph: *** NONE *** (CRITICAL ERROR)")
    
    # Check Activated
    try:
        activated = comp.get_editor_property("activated")
        print(f"  Activated: {activated}")
    except:
        print(f"  Activated: (Cannot Read)")
    
    # Check Generation Trigger
    try:
        trigger = comp.get_editor_property("generation_trigger")
        print(f"  Trigger: {trigger}")
    except:
        pass

# === 4. PCG GRAPH NODE CHECK ===
print("\\n[4] PCG GRAPH NODES")
print("-"*40)

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print(f"ERROR: Cannot load graph {graph_path}")
else:
    print(f"Graph: {graph.get_name()}")
    print(f"Node Count: {len(graph.nodes)}")
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        class_name = settings.get_class().get_name()
        
        if "Sampler" in class_name:
            print(f"\\n  [SAMPLER] {class_name}")
            try:
                ppsm = settings.get_editor_property("points_per_square_meter")
                print(f"    Points Per Square Meter: {ppsm}")
            except: pass
            try:
                unbounded = settings.get_editor_property("unbounded")
                print(f"    Unbounded: {unbounded}")
            except: pass
            try:
                looseness = settings.get_editor_property("looseness")
                print(f"    Looseness: {looseness}")
            except: pass
        
        if "StaticMeshSpawner" in class_name:
            print(f"\\n  [SPAWNER] {class_name}")
            # Check mesh selector
            try:
                sel_type = settings.get_editor_property("mesh_selector_type")
                print(f"    Mesh Selector Type: {sel_type}")
            except: pass
            try:
                sel_instance = settings.get_editor_property("mesh_selector_instance")
                if sel_instance:
                    print(f"    Selector Instance: {sel_instance.get_name()}")
                    # Try to get meshes from selector
                    try:
                        meshes = sel_instance.get_editor_property("mesh_entries")
                        print(f"    Mesh Entries: {len(meshes) if meshes else 0}")
                    except: pass
            except: pass
        
        if "GetLandscape" in class_name:
            print(f"\\n  [LANDSCAPE] {class_name}")
            try:
                actor_sel = settings.get_editor_property("actor_selector")
                sel_mode = actor_sel.get_editor_property("actor_selection")
                sel_tag = actor_sel.get_editor_property("actor_selection_tag")
                must_overlap = actor_sel.get_editor_property("must_overlap_self")
                print(f"    Selection Mode: {sel_mode}")
                print(f"    Selection Tag: {sel_tag}")
                print(f"    Must Overlap Self: {must_overlap}")
            except Exception as e:
                print(f"    Error reading selector: {e}")

print("\\n" + "="*60)
print("       DIAGNOSIS COMPLETE")
print("="*60)
"""

def diagnose():
    print(f"--- [Comprehensive PCG Diagnosis] ---", flush=True)
    
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
    diagnose()
