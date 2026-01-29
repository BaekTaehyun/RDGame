import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Grid Coordinate Space (Final) ---")

graph = unreal.load_asset(graph_path)
if graph:
    grid_node = None
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_name():
            grid_node = n
            break
            
    if grid_node:
        s = grid_node.get_settings()
        
        # 1. Identify Correct Enum
        # We need "Relative" or "Original" (not World).
        # Let's inspect enum members via Python reflection on the type if possible, or just guess.
        # unreal.PCGCoordinateSpace.WORLD is 0.
        # unreal.PCGCoordinateSpace.ORIGINAL is likely 1.
        
        target_val = None
        try:
            # Try accessing standard names
            if hasattr(unreal.PCGCoordinateSpace, "ORIGINAL"):
                print("Found Enum: ORIGINAL")
                target_val = unreal.PCGCoordinateSpace.ORIGINAL
            elif hasattr(unreal.PCGCoordinateSpace, "RELATIVE"):
                print("Found Enum: RELATIVE")
                target_val = unreal.PCGCoordinateSpace.RELATIVE
            elif hasattr(unreal.PCGCoordinateSpace, "LOCAL"):
                print("Found Enum: LOCAL")
                target_val = unreal.PCGCoordinateSpace.LOCAL
                
            if target_val is None:
                # Fallback: Assume 1 is what we want (Original/Relative)
                print("Enum member name unknown, forcing Integer 1 cast...")
                target_val = unreal.PCGCoordinateSpace.cast(1)
        except:
             # If all fails
             pass

        if target_val is not None:
             try:
                 s.set_editor_property("CoordinateSpace", target_val)
                 print(f"Set CoordinateSpace to: {target_val}")
             except Exception as e:
                 print(f"Set Error: {e}")
        else:
             print("Could not determine Target Enum.")
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Saved.")

"""

def fix_grid_final():
    print(f"--- [Fix] Grid Final ---", flush=True)
    
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
    fix_grid_final()
