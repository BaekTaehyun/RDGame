import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if graph:
    print(f"--- Node List for {graph.get_name()} ---")
    nodes = sorted(graph.nodes, key=lambda n: n.get_name())
    for n in nodes:
        name = n.get_name()
        if "Spawner" in name:
            # Get Settings 
            s = n.get_settings()
            entries_count = 0
            first_mesh = "None"
            try:
                # Try simple property access for summary
                sel = s.get_editor_property("mesh_selector_parameters")
                entries = sel.get_editor_property("MeshEntries")
                entries_count = len(entries)
                if entries_count > 0:
                    first_mesh = entries[0].get_editor_property("Descriptor").get_editor_property("StaticMesh").get_name()
            except:
                pass
            print(f"[{name}] Entries: {entries_count} (Ex: {first_mesh})")
else:
    print("Graph not found")
"""

def list_nodes():
    print(f"--- [Info] Listing Nodes for User Alignment ---", flush=True)
    proc = subprocess.Popen([sys.executable, BRIDGE_SCRIPT], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=sys.stderr, text=True, bufsize=0)
    
    def rpc(method, params, expect_response=True):
        req_id = int(time.time()*1000)
        req = {"jsonrpc": "2.0", "method": method, "params": params, "id": req_id}
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
        print(res['result'].get('output', 'No Output'))
        
    except Exception as e:
        print(f"[Error] {e}")
    finally:
        proc.terminate()

if __name__ == "__main__":
    list_nodes()
