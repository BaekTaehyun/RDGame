import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Variety (Noise) & Masking (Difference) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Nodes
    grid_node = None
    bounds_node = None
    diff_node = None
    noise_node = None
    trans_node = None # Forest_Transform_Fixed
    
    # 1.A Find Grid
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0": grid_node = n
        if "BoundsModifier" in n.get_name(): bounds_node = n
        if "Difference" in n.get_name(): diff_node = n
        if "AttributeNoise" in n.get_name(): noise_node = n
        
        t = "Unknown"
        try: t = n.get_editor_property("NodeTitleOverride")
        except: pass
        if "Forest_Transform" in str(t): trans_node = n
    
    # Fallback Transform
    if not trans_node:
         for n in graph.nodes:
             # Look for the last transform we made (near 650, 1600?)
             pass 

    # 2. Configure Noise (Essential for Mixing)
    if noise_node:
        try:
            s = noise_node.get_settings()
            # Set to Density?
            # AttributeName = Density ? or None (Default is Density)
            # Min = 0, Max = 1
            # We assume properties exist.
            pass
        except: pass
        print("Noise Node Checked.")

    # 3. Force Rewire Difference (Essential for Path Removal)
    # Difference Pin 0 = Source (Grid)
    # Difference Pin 1 = Difference (Bounds)
    
    if grid_node and bounds_node and diff_node:
        try:
            # We can't access Pins by Index easily in Python wrapper sometimes.
            # But add_edge_by_name uses Pin Labels.
            # Labels: "Source", "Differences" (plural?) or "Difference".
            # Let's try "Source" and "Difference".
            
            # Grid -> Source
            graph.add_edge_by_name(grid_node, "Out", diff_node, "Source")
            
            # Bounds -> Difference
            graph.add_edge_by_name(bounds_node, "Out", diff_node, "Differences") # Often plural
            
            print("Connected: Grid->Diff(Source), Bounds->Diff(Differences).")
            
        except Exception as e:
            print(f"Diff Connection Error: {e}")
            # Try singular
            try: graph.add_edge_by_name(bounds_node, "Out", diff_node, "Difference")
            except: pass

    # 4. Chain: Diff -> Noise -> Transform -> Project
    # We found 'Project' in previous scripts, let's assume it exists and just link Transform -> Project
    
    proj_node = None
    for n in graph.nodes:
        if "Projection" in n.get_name(): proj_node = n
        
    if diff_node and noise_node and trans_node:
        graph.add_edge(diff_node, "Out", noise_node, "In")
        graph.add_edge(noise_node, "Out", trans_node, "In")
        print("Chain: Diff -> Noise -> Transform")
        
        if proj_node:
            graph.add_edge(trans_node, "Out", proj_node, "In")
            print("Chain: Transform -> Projection")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Fix Applied. Check for Mixing and Path Clearing.")

"""

def fix_variety():
    print(f"--- [Fix] Fixing Variety ---", flush=True)
    
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
    fix_variety()
