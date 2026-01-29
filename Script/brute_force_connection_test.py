import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Brute Force] Connection Discovery ---")

graph = unreal.load_asset(graph_path)
if graph:
    land = None
    sampler = None
    union = None
    bounds = None
    diff = None
    
    for n in graph.nodes:
        nm = n.get_name()
        if "GetLandscape" in nm: land = n
        if "SurfaceSampler" in nm: sampler = n
        if "Union" in nm: union = n
        if "BoundsModifier" in nm: bounds = n
        if "Difference" in nm: diff = n

    def try_pair(src, dst, label):
        # Define candidates locally
        outs_local = ["Out", "Output", "Data", "Result", "Source"]
        ins_local = ["In", "Input", "Surface", "Source", "Target", "Difference", "Differences", "Standard", "Data"]

        if not src or not dst:
            print(f"Skipping {label} (Missing nodes)")
            return
            
        print(f"Testing {label} ({src.get_name()} -> {dst.get_name()})...")
        found = False
        for s in outs_local:
            for d in ins_local:
                try:
                    # must cast to Name
                    res = graph.add_edge(src, unreal.Name(s), dst, unreal.Name(d))
                    if res:
                        print(f"  [SUCCESS] {s} -> {d}")
                        found = True
                        break
                except: pass
            if found: break
        if not found:
            print(f"  [FAILED] Could not connect {label}")

    # 1. Land -> Sampler
    try_pair(land, sampler, "Landscape -> Sampler")
    
    # 2. Union -> Bounds
    try_pair(union, bounds, "Union -> Bounds")
    
    # 3. Bounds -> Difference
    try_pair(bounds, diff, "Bounds -> Difference")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    # Sync
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def brute_force_connect():
    print(f"--- [Brute Force] Connect ---", flush=True)
    
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
    brute_force_connect()
