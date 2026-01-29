import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] Verifying Readers ---")

graph = unreal.load_asset(graph_path)
if graph:
    for n in graph.nodes:
        # Check title and name
        title = "Unknown"
        try: title = n.get_node_title() # API check required?
        except: 
            try: title = n.node_title_override 
            except: pass
        
        name = n.get_name()
        settings = n.get_settings()
        s_name = settings.get_class().get_name() if settings else "NoSettings"
        
        if "Reader" in name or "Reader" in title or "Reader" in s_name:
            print(f"Found Reader Node: {name} (Type: {s_name})")
            
            # Check Position
            try:
                # Direct prop check
                px = n.get_editor_property("NodePosX")
                py = n.get_editor_property("NodePosY")
                print(f"  Pos: ({px}, {py})")
            except:
                try: 
                    # fallback
                    pos = n.get_node_position() # if API exists
                    print(f"  Pos: {pos}")
                except:
                    print("  Pos: Cannot read")
            
            # Check Edges (Outbound)
            # We can't easily iter edges from node in Python?
            # We must verify if the graph has an edge FROM this node.
            # graph.get_edges() ? Not usually exposed.
            # But the topology script iterated something?
            # Topology script likely assumed edge logic or just printed nodes.
            # Let's just print basic info first.
"""

def verify_readers():
    print(f"--- [Audit] Verifying Readers ---", flush=True)
    
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
    verify_readers()
