import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Replacing Broken Transform (Fixing Point Collapse) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Start Fresh: Find and Delete 'Forest_Transform'
    # Also find Grid and Project to re-link.
    
    grid_node = None
    proj_node = None
    old_trans = None
    
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0": grid_node = n
        
        if "Projection" in n.get_name(): proj_node = n
        
        # Look for the transform we made
        title = "Unknown"
        try: title = n.get_editor_property("NodeTitleOverride")
        except: pass
        if "Forest" in str(title) or (n.get_name() != "TransformPoints_2" and "TransformPoints" in n.get_name()):
             # Crude check but we want to clear the bad one.
             # Ruins is #2.
             if n.get_name() != "TransformPoints_2":
                 old_trans = n

    if old_trans:
        try:
            graph.remove_node(old_trans)
            print("Removed Broken Transform Node.")
        except: pass
        
    # 2. Create NEW Transform (Clean Slate)
    ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
    new_trans = ret[0]
    
    try:
        new_trans.set_editor_property("NodeTitleOverride", "Forest_Transform_Fixed")
        new_trans.set_node_position(1600, 650) # Near Grid
        
        s = new_trans.get_settings()
        # Rotation 0-360
        s.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
        # Offset (Jitter)
        jitter = 150.0
        s.set_editor_property("OffsetMin", unreal.Vector(-jitter, -jitter, 0))
        s.set_editor_property("OffsetMax", unreal.Vector(jitter, jitter, 0))
        # Scale
        s.set_editor_property("ScaleMin", unreal.Vector(0.8, 0.8, 0.8))
        s.set_editor_property("ScaleMax", unreal.Vector(1.4, 1.4, 1.4))
        
        # DO NOT SET ABSOLUTE! Default is Relative.
        print("New Transform Configured (Relative Jitter).")
    except Exception as e:
        print(f"Transform Config Error: {e}")

    # 3. Re-Link: Grid -> Transform -> Project -> Filters
    if grid_node and proj_node and new_trans:
        
        # Grid -> Transform
        try: graph.add_edge(grid_node, "Out", new_trans, "In")
        except: pass
        
        # Transform -> Projection
        try: graph.add_edge(new_trans, "Out", proj_node, "In")
        except: pass
        
        # Projection -> Filters (Already connected? No, we broke chain if Project was fed by Old Trans?)
        # My previous script linked Project -> Filters.
        # But wait, did I link Project -> Filters? Yes in Step 6107.
        # But if 'Project' input was 'Trans', removing Trans breaks Project Input.
        # So we just re-feed Project Input. Outputs of Project remain? Yes.
        
        print("Chain Repair: Grid -> NewTransform -> Projection.")
        
    else:
        print("Missing Nodes for Linkage.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Fixed. Points should be spread out now.")

"""

def fix_transform_collapse():
    print(f"--- [Fix] Fixing Collapse ---", flush=True)
    
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
    fix_transform_collapse()
