import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] Inspecting Dungeon Readers ---")

graph = unreal.load_asset(graph_path)
if graph:
    for n in graph.nodes:
        if "DungeonDataReader" in n.get_name():
            print(f"Node: {n.get_name()}")
            s = n.get_settings()
            # Inspect properties to find "Wall" or "Floor"
            # Usually 'Query' or 'Selection'
            # Let's print all properties that look relevant
            # Or just print the string representation of the settings to find keywords
            print(f"  Settings: {s}")
            
            # Try to get specific props if possible
            # Common: "Criteria", "MarkerName", "Tag"
            # Let's try to list properties
            # dir(s) is useful
            # But let's look for "Wall" in the string output first.
            
"""

def inspect_readers():
    print(f"--- [Audit] Readers ---", flush=True)
    
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
    inspect_readers()
