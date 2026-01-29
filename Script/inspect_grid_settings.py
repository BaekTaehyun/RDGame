import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Inspect] Grid Settings ---")

graph = unreal.load_asset(graph_path)
if graph:
    grid_node = None
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_name():
            grid_node = n
            break
            
    if grid_node:
        s = grid_node.get_settings()
        
        # GridExtents
        ext = s.get_editor_property("GridExtents")
        print(f"GridExtents: {ext}")
        
        # CoordinateSpace
        # It's an Enum property. Printing it usually shows "EnumName.Value (Index)" or similar.
        try:
            space = s.get_editor_property("CoordinateSpace")
            print(f"CoordinateSpace Value: {space}")
            # Try to get string representation
            print(f"CoordinateSpace Type: {type(space)}")
        except Exception as e:
            print(f"Read Error: {e}")
    else:
        print("Grid Node Not Found!")

"""

def inspect_grid():
    print(f"--- [Inspect] Grid ---", flush=True)
    
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
    inspect_grid()
