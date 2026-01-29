import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def verify_range():
    print(f"--- [Verification] Checking Filter Range ---", flush=True)
    
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
        
        # Check Filter via Get Properties (Robust)
        res = rpc("tools/call", {
            "name": "get_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "DensityFilter_1",
                "property_names": ["LowerBound", "UpperBound"]
            }
        }, True)
        
        print(f"   Result: {res['result']['content'][0]['text']}", flush=True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    verify_range()
