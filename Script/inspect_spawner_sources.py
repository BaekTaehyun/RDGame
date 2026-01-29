import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] Tracing Spawner Sources ---")

graph = unreal.load_asset(graph_path)
if graph:
    # We want to find what feeds Spawner 0 and 1.
    # Since we can't do graph traversal easily in Python default API,
    # we'll look for nodes that are likely connected based on Position Y or naming conventions.
    # Or just listed Filters.
    
    # Topology Dump from earlier:
    # Spawner_0 Y: -256
    # Spawner_1 Y: 48
    
    # Filter_1 Y: -128 (Between? or Close?)
    # Filter_2 Y: 112 (Close to 48?)
    
    print("Listing Filters and Spawners to correlate:")
    
    nodes = []
    for n in graph.nodes:
        name = n.get_name()
        if "Spawner" in name or "Filter" in name:
            pos_y = "Unknown"
            try: pos_y = n.get_editor_property("NodePosY")
            except: 
                try: pos_y = n.position_y
                except: pass
            
            print(f"  {name} : Y={pos_y}")
            
    print("Hypothesis: AttributeFilter_1 feeds Spawner_0, AttributeFilter_2 feeds Spawner_1")
"""

def inspect_sources():
    print(f"--- [Audit] Tracing Sources ---", flush=True)
    
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
    inspect_sources()
