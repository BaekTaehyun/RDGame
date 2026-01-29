import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Debug] Enable Node Visualization ---")

graph = unreal.load_asset(graph_path)
if graph:
    # We want to enable debug on: Wall Reader, Grid, Copy, Lift, Projection
    target_nodes = ["DungeonDataReader", "CreatePointsGrid", "CopyPoints", "TransformPoints", "Projection"]
    
    count = 0
    for n in graph.nodes:
        nm = n.get_name()
        match = False
        for t in target_nodes:
            if t in nm:
                match = True
                break
        
        if match:
            # Enable Debug
            try:
                # bDebug is usually on the node itself
                n.set_editor_property("bDebug", True)
                print(f"Enabled Debug on: {nm}")
                count += 1
            except Exception as e:
                print(f"Failed to set debug on {nm}: {e}")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print(f"Enabled Debug on {count} nodes.")
    
    # Sync
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def enable_debug():
    print(f"--- [Debug] Enable Viz ---", flush=True)
    
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
    enable_debug()
