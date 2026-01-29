import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Inspect] Reader Settings ---")

graph = unreal.load_asset(graph_path)
if graph:
    for n in graph.nodes:
        if "DungeonDataReader" in n.get_name():
            print(f"\\nNode: {n.get_name()}")
            s = n.get_settings()
            
            # Inspect properties
            try:
                # Common property names for Dungeon selection
                props = ["TargetTileType", "SelectionKey", "Selector", "QueryType"]
                for p in props:
                    try:
                        val = s.get_editor_property(p)
                        print(f"  {p}: {val}")
                    except:
                        # Try to handle Enum
                        pass
                        
                # Just print all properties via Dir?? No, too noisy.
                # Let's try to infer from 'NodeTitleOverride' what it SHOULd be.
                title = ""
                try: title = n.get_editor_property("NodeTitleOverride")
                except: pass
                print(f"  Title: '{title}'")
            except: pass
"""

def inspect_reader():
    print(f"--- [Inspect] Reader ---", flush=True)
    
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
    inspect_reader()
