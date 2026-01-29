import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Check Spawned Mesh Instances ===")

world = unreal.EditorLevelLibrary.get_editor_world()

# Count ISM/HISM components in the level
ism_count = 0
hism_count = 0
smc_count = 0

for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    isms = actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    hisms = actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    smcs = actor.get_components_by_class(unreal.StaticMeshComponent)
    
    for ism in isms:
        count = ism.get_instance_count()
        if count > 0:
            ism_count += count
            print(f"ISM on {actor.get_name()}: {count} instances")
    
    for hism in hisms:
        count = hism.get_instance_count()
        if count > 0:
            hism_count += count
            # Only show first few
            if hism_count < 100:
                print(f"HISM on {actor.get_name()}: {count} instances, Mesh: {hism.get_editor_property('static_mesh')}")

print(f"\\nTotal ISM Instances: {ism_count}")
print(f"Total HISM Instances: {hism_count}")

# Check the PCG actors specifically
print("\\n=== PCG Generated Actors ===")
pcg_actors = unreal.GameplayStatics.get_all_actors_with_tag(world, "PCG_GeneratedActor")
print(f"PCG Generated Actors with tag: {len(pcg_actors)}")

# Check ALL actors that might be trees
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    name = actor.get_name().lower()
    if 'tree' in name or 'foliage' in name or 'bush' in name:
        print(f"Found: {actor.get_name()} at {actor.get_actor_location()}")

print("\\n=== Done ===")
"""

def check_spawned():
    print(f"--- [Check Spawned Instances] ---", flush=True)
    
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
    check_spawned()
