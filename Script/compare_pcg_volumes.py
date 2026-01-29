import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Compare PCG Volumes ===")

world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PCGVolume)

for vol in actors:
    print(f"\\n--- {vol.get_actor_label()} ---")
    print(f"  Name: {vol.get_name()}")
    print(f"  Location: {vol.get_actor_location()}")
    print(f"  Scale: {vol.get_actor_scale3d()}")
    
    # Check Tags
    tags = vol.tags
    print(f"  Tags: {[str(t) for t in tags]}")
    
    # Brush Component
    brush = vol.brush_component
    if brush:
        print(f"  BrushComponent: {brush.get_name()}")
        print(f"    - Bounds: {brush.bounds}")
        print(f"    - IsRegistered: {brush.is_registered()}")
        print(f"    - IsVisible: {brush.is_visible()}")
        
        # Check if Brush has valid geometry (Model)
        # This is tricky in Python API
        try:
            body_setup = brush.get_body_setup()
            print(f"    - BodySetup: {body_setup}")
        except:
            print(f"    - BodySetup: N/A")
    else:
        print(f"  BrushComponent: NONE!")
        
    # PCG Component
    pcg = vol.get_component_by_class(unreal.PCGComponent)
    if pcg:
        print(f"  PCGComponent: {pcg.get_name()}")
        print(f"    - Graph: {pcg.graph}")
        print(f"    - IsActivated: {pcg.b_activated}")
    else:
        print(f"  PCGComponent: NONE!")

print("\\n=== Done ===")
"""

def compare_pcg_volumes():
    print(f"--- [Compare PCG Volumes] ---", flush=True)
    
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
    compare_pcg_volumes()
