import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal
import json

print("=== PCG Full Graph Dump ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    graph_data = {
        "nodes": [],
        "edges": [] # We can't iterate edges easily in Python API unfortunately
    }
    
    node_map = {}
    
    # Dump Nodes
    for node in graph.nodes:
        # Node ID/Name
        node_name = node.get_name()
        node_title = str(node.node_title)
        
        # Settings
        settings = node.get_settings()
        settings_data = {}
        s_class = "None"
        
        if settings:
            s_class = settings.get_class().get_name()
            # Dump properties
            for p in dir(settings):
                if p.startswith('_') or callable(getattr(settings, p)): continue
                try:
                    val = getattr(settings, p)
                    # Simple types only
                    if isinstance(val, (bool, int, float, str)):
                        settings_data[p] = val
                    elif isinstance(val, unreal.Name):
                        settings_data[p] = str(val)
                    elif isinstance(val, unreal.Vector):
                        settings_data[p] = f"({val.x},{val.y},{val.z})"
                except: pass
                
        n_data = {
            "name": node_name,
            "title": node_title,
            "class": s_class,
            "settings": settings_data
        }
        graph_data["nodes"].append(n_data)
        node_map[node] = node_name

    # Check Connections (Heuristic)
    # Since we can't iterate Edges (PCGGraph.get_edges() missing?),
    # We can try to infer from 'downstream_nodes' or similar if available?
    # No, usually we rely on "add_edge" success.
    # But wait, earlier I used 'graph.add_edge' but didn't check if they persist.
    
    # We can try to print the 'InputPins' and 'OutputPins' of each node?
    # PCGNode has 'get_input_pins()' ?
    
    print(json.dumps(graph_data, indent=2))

print("\\n=== Done ===")
"""

def dump_pcg_graph():
    print(f"--- [Dump PCG Graph] ---", flush=True)
    
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
        
        # Output might be large, just print simple success message or extract JSON
        out = res.get('result', {}).get('content', [{'text': 'No Output'}])[0]['text']
        print("Done.")
        # We want to see the output though
        print(out)

    except Exception as e:
        print(f"[Error] {e}")
    finally:
        proc.terminate()

if __name__ == "__main__":
    dump_pcg_graph()
