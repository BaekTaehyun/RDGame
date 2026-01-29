import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Sampler Fix & Debug ===")

world = unreal.EditorLevelLibrary.get_editor_world()
graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Modify Sampler Settings
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        cname = settings.get_class().get_name()
        
        if "SurfaceSampler" in cname:
            print(f"\\nModifying {cname}...")
            # Set loose constraints
            try:
                settings.set_editor_property("looseness", 1.0)
                print("  Set Looseness = 1.0")
            except: pass
            
            try:
                settings.set_editor_property("points_per_square_meter", 0.1) # Ensure > 0
                print("  Set PointsPerSqrM = 0.1")
            except: pass
            
            try:
                settings.set_editor_property("unbounded", True)
                print("  Set Unbounded = True")
            except: pass

        if "GetLandscape" in cname:
            print(f"\\nModifying {cname}...")
            try:
                # Debug: Change to 'Get Single Actor' if possible, or verify Tag
                sel = settings.get_editor_property("actor_selector")
                tag = sel.get_editor_property("actor_selection_tag")
                mode = sel.get_editor_property("actor_selection")
                print(f"  Current Mode: {mode}")
                print(f"  Current Tag: {tag}")
                
            except Exception as e:
                print(f"  GetLandscape settings error: {e}")

    # Save Graph
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved!")

# 2. Verify Landscape Actor in World
print("\\n=== Checking Landscape Actor ===")
ls_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)
target_tag = "DungeonGeneratedLandscape"
found_tagged = False

for ls in ls_actors:
    print(f"Landscape: {ls.get_name()}")
    tags = [str(t) for t in ls.tags]
    print(f"  Tags: {tags}")
    
    if target_tag in tags:
        found_tagged = True
    else:
        print(f"  -> ADDING MISSING TAG: {target_tag}")
        ls.tags.append(target_tag)

if found_tagged:
    print("At least one Landscape has the correct Tag.")

# 3. Regen
print("\\n=== Regenerating PCG ===")
# Find PCG Component
pcg_comp = None
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    comps = actor.get_components_by_class(unreal.PCGComponent)
    if comps:
        pcg_comp = comps[0]
        pcg_comp.generate_local(True)
        print(f"Regenerated {pcg_comp.get_name()}")
        
        # Check Output immediately? (Might be async)
        # But let's try
        try:
            data = pcg_comp.get_generated_graph_output()
            if data and hasattr(data, 'tagged_data'):
                print(f"Output Tagged Data Count: {len(data.tagged_data)}")
        except: pass

print("\\n=== Done ===")
"""

def fix_sampler():
    print(f"--- [Fix PCG Sampler] ---", flush=True)
    
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
    fix_sampler()
