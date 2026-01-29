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
    print(f"--- Topology Audit for {{graph.get_name()}} ---")
    nodes = sorted(graph.nodes, key=lambda n: n.get_name())
    
    for n in nodes:
        name = n.get_name()
        # Only care about Spawners and Filters
        cls_name = n.get_class().get_name()
        if "Spawner" in name or "Filter" in name:
            pos_x = n.node_position_x
            pos_y = n.node_position_y
            
            # Check Inputs
            inputs = []
            # This is hard in Python without iterating entire graph edges usually.
            # But PCGNode might have 'get_input_pins'?
            # Actually, PCGGraph has 'get_upstream_nodes' or similar?
            # Let's just print Position for now.
            
            print(f"[Node: {{name}}] Pos: ({{pos_x}}, {{pos_y}})")

    # Also count total edges check?
    # graph.get_all_connections()?
else:
    print("Graph not found")
"""

def inspect_topology():
    print(f"--- [Audit] Checking Node Positions ---", flush=True)
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
    inspect_topology()
