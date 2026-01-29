import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Resetting Filters to 0.0 ---")

graph = unreal.load_asset(graph_path)
if graph:
    # Target all filters
    targets = []
    for i in range(1, 6):
        targets.append(f"DensityFilter_{i}")
        
    for tname in targets:
        for n in graph.nodes:
            if n.get_name() == tname:
                try:
                    s = n.get_settings()
                    try: s.lower_bound = 0.0
                    except: s.set_editor_property("LowerBound", 0.0)
                    
                    try: s.upper_bound = 1.0
                    except: s.set_editor_property("UpperBound", 1.0)
                    
                    print(f"Reset {tname} -> [0.0, 1.0]")
                except Exception as e:
                    print(f"Error {tname}: {e}")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Filters Relaxed. Check Vis.")

"""

def reset_filters():
    print(f"--- [Fix] Resetting ---", flush=True)
    
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
    reset_filters()
