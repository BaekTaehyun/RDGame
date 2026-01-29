import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Inspect] PCG Classes ---")

# List all attributes in unreal module that start with PCG
pcg_classes = [x for x in dir(unreal) if x.startswith("PCG")]
landscape_classes = [x for x in pcg_classes if "Landscape" in x]
data_classes = [x for x in pcg_classes if "Data" in x]

print(f"\\nLandscape Classes ({len(landscape_classes)}):")
for c in landscape_classes:
    print(f"  - {c}")

print(f"\\nData Classes ({len(data_classes)}):")
# Filter to keep list short
relevant_data = [x for x in data_classes if "Get" in x or "Source" in x or "Settings" in x]
for c in relevant_data:
    print(f"  - {c}")
"""

def inspect_classes():
    print(f"--- [Inspect] Classes ---", flush=True)
    
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
    inspect_classes()
