import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Applying Tiered Density (Ecotone) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # We apply staggered thresholds to create a transition zone.
    # Higher LowerBound = Only spawns in high-density areas (Deep Forest).
    # Lower LowerBound = Spawns in low-density areas too (Edge of Forest).
    
    # Layer 0: Big Trees (Should be Sparse & Deep)
    # Layer 1: Medium (More common)
    # Layer 2: Small/Bush (Most common, closer to edge)
    
    tiers = [
        ("DensityFilter_1", 0.85), # Layer 0: Only top 15% density (Deep/Sparse)
        ("DensityFilter_2", 0.60), # Layer 1: Top 40%
        ("DensityFilter_3", 0.40), # Layer 2: Top 60% (Spreads out)
        ("DensityFilter_4", 0.30)  # Layer 3: Top 70% (Ground cover)
    ]
    
    for fname, bound in tiers:
        found = False
        for n in graph.nodes:
            if n.get_name() == fname:
                found = True
                try:
                    s = n.get_settings()
                    # Apply
                    try: s.lower_bound = bound
                    except: s.set_editor_property("LowerBound", bound)
                    
                    print(f"Set {fname} LowerBound -> {bound}")
                except Exception as e:
                    print(f"Error {fname}: {e}")
                break
        if not found:
            print(f"Warning: {fname} not found")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Tiered Density Applied. Edge should be softer.")

"""

def tune_tiers():
    print(f"--- [Fix] Tuning Tiers ---", flush=True)
    
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
    tune_tiers()
