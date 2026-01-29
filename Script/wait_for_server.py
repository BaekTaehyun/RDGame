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
    print(f"--- Topology Audit for {graph.get_name()} ---")
    nodes = sorted(graph.nodes, key=lambda n: n.get_name())
    
    for n in nodes:
        name = n.get_name()
        # Only care about Spawners and Filters
        cls_name = n.get_class().get_name()
        if "Spawner" in name:
            pos_x = n.node_position_x
            pos_y = n.node_position_y
            print(f"[Node: {name}] Pos: ({pos_x}, {pos_y})")
else:
    print("Graph not found")
"""

def wait_and_inspect():
    print(f"--- [Wait] Waiting for Server... ---", flush=True)
    
    connected = False
    for i in range(20): # Wait 60 seconds (20 * 3)
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
            req = {"jsonrpc": "2.0", "method": method, "params": params, "id": req_id}
            try:
                proc.stdin.write(json.dumps(req) + "\n")
                proc.stdin.flush()
            except: return None
            if expect_response: return json.loads(proc.stdout.readline())
            return None

        try:
            # Try handshake
            rpc("initialize", {}, True)
            rpc("notifications/initialized", {}, False)
            
            # Ping
            ping_res = rpc("tools/call", {"name": "execute_unreal_script", "arguments": {"code": "print('Ping')"}}, True)
            
            if ping_res and not ping_res.get('error'):
                print(f"--- [Connected] Server is Up! ---")
                
                # Check Topology
                print("Running Topology Check...")
                res = rpc("tools/call", {
                    "name": "execute_unreal_script",
                    "arguments": {"code": PYTHON_CODE}
                }, True)
                
                out = res['result'].get('output', 'No Output')
                print(out)
                connected = True
                proc.terminate()
                break
            
        except Exception:
            # print(".", end="", flush=True)
            pass
        
        proc.terminate()
        time.sleep(3)

    if not connected:
        print("\n[Timeout] Server did not come online.")

if __name__ == "__main__":
    wait_and_inspect()
