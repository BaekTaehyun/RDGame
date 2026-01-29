import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Verify Graph State ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    debug_nodes = []
    sampler_nodes = []
    
    for node in graph.nodes:
        t = str(node.node_title)
        c = node.get_settings().get_class().get_name()
        
        if "DEBUG" in t:
            debug_nodes.append(t)
            
        if "SurfaceSampler" in c:
            try:
                unbounded = node.get_settings().get_editor_property("unbounded")
                sampler_nodes.append(f"{t}: Unbounded={unbounded}")
            except:
                sampler_nodes.append(f"{t}: Error reading unbounded")

    print(f"Debug Nodes Found: {debug_nodes}")
    print(f"Sampler Nodes: {sampler_nodes}")

    if not debug_nodes and sampler_nodes:
        print("STATUS: CLEAN (Native Logic Active)")
    else:
        print("STATUS: DIRTY or BROKEN")

print("=== Done ===")
"""

def verify_graph_state():
    print(f"--- [Verify Graph State] ---", flush=True)
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
        req = {"jsonrpc": "2.0", "method": method, "params": params}
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
    verify_graph_state()
