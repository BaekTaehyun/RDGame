import sys
import json
import subprocess
import time
import os

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
OUTPUT_FILE = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Temp_PCG_Debug.txt"

PYTHON_CODE = f"""
import unreal
import os

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
out_path = r"{OUTPUT_FILE}"

with open(out_path, "w", encoding='utf-8') as f:
    f.write(f"--- PCG Debug Report: {{graph_path}} ---\\n")
    
    try:
        graph = unreal.load_asset(graph_path)
        if not graph:
            f.write("Error: Graph asset not found\\n")
        else:
            nodes = sorted(graph.nodes, key=lambda n: n.get_name())
            f.write(f"Total Nodes: {{len(nodes)}}\\n")
            
            target_node = None
            
            for n in nodes:
                name = n.get_name()
                f.write(f"   [Node] {{name}} ({{n.get_class().get_name()}})\\n")
                if "Spawner" in name:
                    target_node = n
            
            if target_node:
                f.write("\\n--- Properties of Spawner Node ---\\n")
                props = dir(target_node)
                for p in props:
                    if "pos" in p.lower() or "node" in p.lower() or "x" == p.lower():
                        try:
                            val = getattr(target_node, p)
                            f.write(f"   {{p}}: {{val}}\\n")
                        except:
                            f.write(f"   {{p}}: <Error getting value>\\n")
            
    except Exception as e_fatal:
        f.write(f"Fatal Debug Error: {{e_fatal}}\\n")

print(f"Debug written to: {{out_path}}")
"""

def debug_node_api():
    print(f"--- [Audit] Debugging Node API ---", flush=True)
    
    if os.path.exists(OUTPUT_FILE):
        try: os.remove(OUTPUT_FILE)
        except: pass

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
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        res = rpc("tools/call", {
            "name": "execute_unreal_script",
            "arguments": {"code": PYTHON_CODE}
        }, True)
        
        time.sleep(1.0)
        
        if os.path.exists(OUTPUT_FILE):
            print("\n--- FILE CONTENT ---")
            with open(OUTPUT_FILE, "r", encoding='utf-8') as f:
                print(f.read())
        else:
            print("[Error] Output file was NOT created.")

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    debug_node_api()
