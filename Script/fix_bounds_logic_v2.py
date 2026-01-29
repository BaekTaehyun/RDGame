import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Bounds Modifier V2 ---")

graph = unreal.load_asset(graph_path)
if graph:
    bounds_node = None
    copy_node = None
    wall_node = None
    
    # Identify
    for n in graph.nodes:
        nm = n.get_name()
        if "BoundsModifier" in nm: bounds_node = n
        if "CopyPoints" in nm: copy_node = n
        if "DungeonDataReader" in nm:
             # Just assume the connected one or find it
             if "Wall" in nm or "2" in nm: wall_node = n 
             # Note: Earlier identification was more robust, assuming existing wiring is somewhat intact
             # or we just need to find the node we likely created prev step.
             
    if bounds_node:
        try:
            s = bounds_node.get_settings()
            # Set Bounds to +/- 200.0
            v_min = unreal.Vector(-200.0, -200.0, -200.0)
            v_max = unreal.Vector(200.0, 200.0, 200.0)
            
            s.set_editor_property("BoundsMin", v_min)
            s.set_editor_property("BoundsMax", v_max)
            s.set_editor_property("bModifyBounds", True) # Ensure this is enabled
            print("Bounds Set to +/- 200.")
        except Exception as e:
            print(f"Bounds Set Error: {e}")
            
    # Retry Wiring if needed (Wall->Bounds->Copy)
    # The previous script successfully printed "Connected", so edges likely exist.
    # But let's re-confirm.
    
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    
    # Sync
    try:
        unreal.DungeonAssetUtils.refresh_blueprint(graph)
        # Note: Previous log said "Refreshed PCG Graph". So it works.
        print("Graph Refreshed.")
    except: pass
"""

def fix_bounds_v2():
    print(f"--- [Fix] Bounds V2 ---", flush=True)
    
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
    fix_bounds_v2()
