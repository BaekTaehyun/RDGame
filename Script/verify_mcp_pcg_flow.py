import sys
import json
import subprocess
import time

# Path to the MCP bridge script
BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

def run_mcp_test():
    print("--- Starting MCP Bridge Verification ---")
    
    # Start the bridge process
    # We communicate via stdin/stdout
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr, # Keep error visible
        text=True,
        bufsize=0 # Unbuffered
    )
    
    def send_json_rpc(method, params=None, msg_id=1):
        req = {
            "jsonrpc": "2.0",
            "method": method,
            "params": params or {},
            "id": msg_id
        }
        json_str = json.dumps(req)
        # print(f"[Client -> Bridge] {json_str}")
        proc.stdin.write(json_str + "\n")
        proc.stdin.flush()
        
    def read_json_response():
        line = proc.stdout.readline()
        if line:
            # print(f"[Bridge -> Client] {line.strip()}")
            return json.loads(line)
        return None

    try:
        # 1. Initialize
        send_json_rpc("initialize", {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "TestClient", "version": "1.0"}
        }, 1)
        read_json_response() # Result
        send_json_rpc("notifications/initialized", {}, 2)
        
        # 2. List Tools
        send_json_rpc("tools/list", {}, 3)
        res_list = read_json_response()
        
        found_inspect = False
        if "result" in res_list and "tools" in res_list["result"]:
             for t in res_list["result"]["tools"]:
                 if t["name"] == "inspect_pcg_graph":
                     found_inspect = True
                     break
        
        if found_inspect:
            print("[PASS] Tool 'inspect_pcg_graph' found in list.")
        else:
            print("[FAIL] Tool 'inspect_pcg_graph' missing!")
            return

        # 3. Call Inspect Tool
        target_graph = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
        print(f"\n--- Testing 'inspect_pcg_graph' on {target_graph} ---")
        
        send_json_rpc("tools/call", {
            "name": "inspect_pcg_graph",
            "arguments": {
                "graph_path": target_graph
            }
        }, 4)
        
        res_inspect = read_json_response()
        if not res_inspect or "result" not in res_inspect:
            print(f"[FAIL] Invalid Response: {res_inspect}")
            return

        content_str = res_inspect["result"]["content"][0]["text"]
        
        # Handle embedded JSON text
        try:
            inspect_data = json.loads(content_str)
            if "data" in inspect_data:
                # It's double escaped JSON from C++
                topology = json.loads(inspect_data["data"])
                print(f"[SUCCESS] Got Topology with {len(topology['Nodes'])} Nodes.")
                
                # Verify Ruins Chain
                print("\n[Topology Check]")
                for n in topology['Nodes']:
                    name = n['Name']
                    outs = n['Outbound']
                    if "Ruins" in name or "Ruins" in n.get('Title', ''):
                         print(f" - {name} ({n.get('Title','')}) -> {outs}")
                         
            elif "nodes" in inspect_data:
                # Fallback Python
                print(f"[SUCCESS] Got Python Node List: {len(inspect_data['nodes'])} Nodes")
            else:
                print(f"[FAIL] Inspect returned unexpected data: {inspect_data.keys()}")
                
        except json.JSONDecodeError:
            print(f"[FAIL] Check bridge output format. Raw: {content_str[:100]}...")

    except Exception as e:
        print(f"[ERROR] {e}")
    finally:
        proc.terminate()
        print("\n--- Verification Finished ---")

if __name__ == "__main__":
    run_mcp_test()
