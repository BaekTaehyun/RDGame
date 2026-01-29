import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal
import json

print("=== PCG Full Graph Dump (Robust) ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    graph_data = {
        "nodes": []
    }
    
    # Dump Nodes
    for node in graph.nodes:
        # Node ID/Name
        node_name = node.get_name()
        # Convert Name to str explicitly
        node_title = str(node.node_title) 
        
        # Settings
        settings = node.get_settings()
        settings_data = {}
        s_class = "None"
        
        if settings:
            s_class = settings.get_class().get_name()
            # Dump properties
            for p in dir(settings):
                if p.startswith('_'): continue
                
                # Check callability safely?
                try:
                    attr = getattr(settings, p)
                    if callable(attr): continue
                    
                    # Protected Check?
                    # Just Try-Catch
                    
                    val = attr
                    # Simple types only
                    if isinstance(val, (bool, int, float, str)):
                        settings_data[p] = val
                    elif isinstance(val, unreal.Name):
                        settings_data[p] = str(val)
                    elif isinstance(val, unreal.Vector):
                        settings_data[p] = f"({val.x},{val.y},{val.z})"
                    # Enums are usually ints or special objects, simpler to skip complex ones
                    elif isinstance(val, list):
                        settings_data[p] = f"List(len={len(val)})"
                except: 
                    # Skip protected/private properties
                    pass
                
        n_data = {
            "name": node_name,
            "title": node_title,
            "class": s_class,
            "settings": settings_data
        }
        graph_data["nodes"].append(n_data)

    print(json.dumps(graph_data, indent=2))

print("\\n=== Done ===")
"""

def dump_pcg_robust():
    print(f"--- [Dump PCG Robust] ---", flush=True)
    
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
        
        out = res.get('result', {}).get('content', [{'text': 'No Output'}])[0]['text']
        print(out)

    except Exception as e:
        print(f"[Error] {e}")
    finally:
        proc.terminate()

if __name__ == "__main__":
    dump_pcg_robust()
