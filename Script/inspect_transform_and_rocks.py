import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Audit] Inspecting Transform & Assets ---")

# 1. Inspect Transform Settings Class
obj = unreal.PCGTransformPointsSettings()
print(f"Class: {obj.get_class().get_name()}")
print("Properties:")
for p in dir(obj):
    if "apply" in p.lower():
        print(f"  {p}")

# 2. Search for Rock Meshes
print("\\n--- Searching for Rocks ---")
# Simple Asset Registry Search
registry = unreal.AssetRegistryHelpers.get_asset_registry()
filter = unreal.ARFilter(
    class_names=["StaticMesh"],
    package_paths=["/Game"],
    recursive_paths=True
)
assets = registry.get_assets(filter)
rock_count = 0
for a in assets:
    name = str(a.asset_name)
    if "rock" in name.lower() or "stone" in name.lower():
        print(f"  Found: {a.package_name}")
        rock_count += 1
        if rock_count > 5: break
"""

def inspect_stuff():
    print(f"--- [Audit] Inspection ---", flush=True)
    
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
    inspect_stuff()
