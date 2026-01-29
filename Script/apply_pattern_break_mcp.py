import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def fix_patterns():
    print(f"--- [Fixing] Breaking Patterns & Exposing Ruins ---", flush=True)
    
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
        if expect_response: return json.loads(proc.stdout.readline())
        return None

    try:
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        # 1. Move Ruins to 'Edges/Open' areas
        # Current: 0.6-1.0 (Deep Forest) -> Invisible inside trees
        # New: 0.05 - 0.3 (Open Fields / Forest Edge)
        print("[1/3] Shifting Ruins to Open Areas (0.05 - 0.3)...", flush=True)
        rpc("tools/call", {
            "name": "set_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "DensityFilter_1",
                "properties": {
                    "LowerBound": 0.05,
                    "UpperBound": 0.3
                }
            }
        }, True)

        # 2. Increase Tree Jitter
        # Current: +/- 65. Cell: 135.
        # Max theoretical shuffle without overlap depends on mesh size, but let's go bold.
        # +/- 90 (Some overlap allowed, breaks grid hard)
        print("[2/3] Increasing Tree Jitter (+/- 90)...", flush=True)
        rpc("tools/call", {
            "name": "set_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "TransformPoints_1",
                "properties": {
                    "offset_min": {"x": -90.0, "y": -90.0, "z": -10.0}, # Slight Z var
                    "offset_max": {"x": 90.0, "y": 90.0, "z": 10.0}
                }
            }
        }, True)

        # 3. Save
        print("[3/3] Saving...", flush=True)
        code_save = f"import unreal; unreal.EditorAssetLibrary.save_asset('{GRAPH_PATH}')"
        rpc("tools/call", {"name": "execute_unreal_script", "arguments": {"code": code_save}}, True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()
        print("\n--- Pattern Break Complete ---", flush=True)

if __name__ == "__main__":
    fix_patterns()
