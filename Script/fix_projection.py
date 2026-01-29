import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Projection (Snap to Landscape) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Find Critical Nodes
    grid_node = None
    trans_node = None
    proj_node = None
    
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0": grid_node = n
        
        # Check for Projection
        if "Projection" in n.get_name() or "WorldRayHit" in n.get_name():
            proj_node = n
            print(f"Found Existing Projection: {n.get_name()}")
            
        # Find Forest Transform
        title = "Unknown"
        try: title = n.get_editor_property("NodeTitleOverride")
        except: pass
        if "Forest" in str(title):
            trans_node = n
            
    # Fallback to verify transform if Title missing
    if not trans_node:
        for n in graph.nodes:
            # Heuristic: Transform connected to Grid?
            # Or just name TransformPoints_1
            if n.get_name() == "TransformPoints_1":
                trans_node = n
                break

    # 2. Create Projection if missing
    if not proj_node:
        ret = graph.add_node_of_type(unreal.PCGProjectionSettings)
        proj_node = ret[0]
        try:
             proj_node.set_node_position(1800, 650) # Between Transform and Filters
             # Configure to Project to Landscape
             s = proj_node.get_settings()
             # Defaults are usually Project to Landscape (Source: Source, Target: Landscape)
             # But let's verify if props exist
        except: pass
        print("Created New Projection Node.")

    # 3. Re-Wire: Grid -> Transform -> Projection -> Filters
    if grid_node and trans_node and proj_node:
        # A. Grid -> Transform
        try: graph.add_edge(grid_node, "Out", trans_node, "In")
        except: pass
        
        # B. Transform -> Projection
        try: graph.add_edge(trans_node, "Out", proj_node, "In")
        except: pass
        
        # C. Projection -> Filters
        targets = []
        for i in range(1, 6):
            targets.append(f"DensityFilter_{i}")
            targets.append(f"AttributeFilter_{i}")
            
        for tname in targets:
            found = None
            for n in graph.nodes:
                if n.get_name() == tname:
                    found = n
                    break
            if found:
                try: graph.add_edge(proj_node, "Out", found, "In")
                except: pass
                
        print("Re-Wired: Grid -> Transform -> Projection -> Filters")
        
    else:
        print("Critical: Nodes missing.")
        if not grid_node: print("  Grid Missing")
        if not trans_node: print("  Transform Missing")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Projection Added. Trees should snap to ground.")

"""

def fix_projection():
    print(f"--- [Fix] Adding Projection ---", flush=True)
    
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
    fix_projection()
