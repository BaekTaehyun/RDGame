import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Audit] Inspecting PCG Graph API ---")
for d in dir(unreal.PCGGraph):
    if any(x in d.lower() for x in ["notify", "refresh", "update", "change", "modify", "broadcast"]):
        print(f"PCGGraph.{d}")

print("--- [Audit] Inspecting PCGSubsystem API ---")
for d in dir(unreal.PCGSubsystem):
    if any(x in d.lower() for x in ["notify", "refresh", "update", "change", "modify"]):
        print(f"PCGSubsystem.{d}")
        
print("--- [Audit] Inspecting Editor Asset Library ---")
for d in dir(unreal.EditorAssetLibrary):
     if any(x in d.lower() for x in ["save", "load", "dirty"]):
        print(f"EditorAssetLibrary.{d}")
"""

def inspect_api():
    print(f"--- [Audit] Running Inspection ---", flush=True)
    
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
    inspect_api()
