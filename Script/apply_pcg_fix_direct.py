import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def fix_rotation_direct():
    print(f"--- [Fixing] Applying Rotation via Direct Script (Kwargs) ---", flush=True)
    
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
        req = {"jsonrpc": "2.0", "method": method, "params": params, "id": req_id}
        try:
            proc.stdin.write(json.dumps(req) + "\n")
            proc.stdin.flush()
        except: return None

        if expect_response:
            return json.loads(proc.stdout.readline())
        return None

    try:
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        py_code = f"""
import unreal
graph_path = "{GRAPH_PATH}"
graph = unreal.load_asset(graph_path)
if graph:
    processed_trees = False
    processed_ruins = False
    
    for n in graph.nodes:
        name = n.get_name()
        
        # Trees
        if name == "TransformPoints_1":
            print(f"Update: {{name}}")
            s = n.get_settings()
            s.modify()
            # Explicit Kwargs
            s.set_editor_property("rotation_max", unreal.Rotator(pitch=0.0, yaw=360.0, roll=0.0))
            s.set_editor_property("uniform_scale", True)
            processed_trees = True
            
        # Ruins
        elif name == "TransformPoints_0":
            print(f"Update: {{name}}")
            s = n.get_settings()
            s.modify()
            s.set_editor_property("rotation_max", unreal.Rotator(pitch=0.0, yaw=360.0, roll=0.0))
            s.set_editor_property("scale_min", unreal.Vector(2.5, 2.5, 2.5))
            s.set_editor_property("scale_max", unreal.Vector(4.5, 4.5, 4.5))
            s.set_editor_property("uniform_scale", True)
            processed_ruins = True
            
    unreal.EditorAssetLibrary.save_asset(graph_path)
    print(f"Trees: {{processed_trees}}, Ruins: {{processed_ruins}}")
else:
    print("Graph not found")
"""

        print("[1/1] Executing Python Fix...", flush=True)
        res = rpc("tools/call", {
            "name": "execute_unreal_script",
            "arguments": {"code": py_code}
        }, True)
        
        print(f"   Result: {res['result'].get('output', 'No Output')}", flush=True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()
        print("\n--- Direct Fix Complete ---", flush=True)

if __name__ == "__main__":
    fix_rotation_direct()
