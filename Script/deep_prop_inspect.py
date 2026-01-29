import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Audit] Deep Property Inspection ---")

# 1. Reflection
obj = unreal.PCGTransformPointsSettings()
uclass = obj.get_class()
print(f"Reflecting {uclass.get_name()}:")

# Iterate properties via Unreal System Library or helper?
variations = ["ApplyRotation", "bApplyRotation", "ApplyPosition", "bApplyPosition", "OffsetMin", "RotationMin", "ScaleMin"]
for v in variations:
    try:
        val = obj.get_editor_property(v)
        print(f"  [MATCH] {v} = {val}")
    except:
        print(f"  [MISS] {v}")

# 2. Asset Search (Nature)
print("\\n--- Searching for Nature Assets ---")
registry = unreal.AssetRegistryHelpers.get_asset_registry()
filter = unreal.ARFilter(
    class_names=["StaticMesh"],
    package_paths=["/Game"],
    recursive_paths=True
)
assets = registry.get_assets(filter)
count = 0
for a in assets:
    try:
        # Careful with FName conversion
        aname = str(a.asset_name).lower()
        pname = str(a.package_name).lower()
        
        if any(x in aname for x in ["rock", "boulder", "stone", "bush", "shrub", "plant", "fern"]):
            # Exclude known structural
            if "floor" not in aname and "wall" not in aname and "pillar" not in aname:
                 print(f"  Maybe Nature: {a.package_name}")
                 count += 1
                 if count > 10: break
    except: pass
"""

def deep_inspect():
    print(f"--- [Audit] Deep Inspection ---", flush=True)
    
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
    deep_inspect()
