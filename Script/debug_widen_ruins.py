import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

print("--- [Debug] Widening Ruins Constraints ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Relax Density Filter (DensityFilter_5)
    filter_node = None
    trans_node = None
    
    for n in graph.nodes:
        name = n.get_name()
        if name == "DensityFilter_5":
            filter_node = n
        elif name == "TransformPoints_2":
            trans_node = n

    if filter_node:
        print("Relaxing Filter (0.0 - 1.0)...")
        settings = filter_node.get_settings()
        try:
            settings.lower_bound = 0.0
            settings.upper_bound = 1.0
        except:
             # Try via set_editor_property if direct access fails
             try:
                 settings.set_editor_property("LowerBound", 0.0)
                 settings.set_editor_property("UpperBound", 1.0)
             except Exception as e:
                 print(f"Failed to set filter: {e}")

    if trans_node:
        print("Resetting Transform Scale (1.0)...")
        settings = trans_node.get_settings()
        try:
             settings.set_editor_property("ScaleMin", unreal.Vector(1,1,1))
             settings.set_editor_property("ScaleMax", unreal.Vector(1,1,1))
             # Also ensure ApplyScale is true
             settings.set_editor_property("ApplyScale", True)
             settings.set_editor_property("UniformScale", True)
        except Exception as e:
             print(f"Failed to set transform: {e}")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Constraints Widened. Please Check Viewport.")
else:
    print("Graph not found")
"""

def debug_widen():
    print(f"--- [Debug] Widening Constraints ---", flush=True)
    
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
    debug_widen()
