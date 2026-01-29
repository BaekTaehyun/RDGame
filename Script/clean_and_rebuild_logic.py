import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Nuclear Cleanup & Rebuild (Copy Points + Sync) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Preserved Nodes & Settings
    nodes_to_keep = []
    nodes_to_delete = []
    
    wall_node = None
    floor_node = None
    filters = []
    spawners = []
    rnd_trans_settings = None
    
    for n in graph.nodes:
        nm = n.get_name()
        keep = False
        
        # Readers
        if "DungeonDataReader" in nm:
            try:
                t = n.get_editor_property("NodeTitleOverride")
                if "Wall" in t: wall_node = n
                elif "Floor" in t: floor_node = n
            except:
                if "2" in nm: wall_node = n
                if "1" in nm: floor_node = n
            keep = True
            
        # Filters & Spawners
        if "DensityFilter" in nm:
            filters.append(n)
            keep = True
        if "StaticMeshSpawner" in nm:
            spawners.append(n)
            keep = True
        if "GraphInput" in nm or "GraphOutput" in nm:
            keep = True
            
        # Capture Random Transform Settings
        if "TransformPoints" in nm:
            try:
                s = n.get_settings()
                min_off = s.get_editor_property("OffsetMin")
                if min_off.z < 500: # It's the randomizer
                    rnd_trans_settings = s
            except: pass
            
        if keep: nodes_to_keep.append(n)
        else: nodes_to_delete.append(n)
            
    # 2. DELETE
    print(f"Deleting {len(nodes_to_delete)} garbage nodes...")
    for n in nodes_to_delete:
         graph.remove_node(n)

    # 3. RE-CREATE
    # Grid (Source)
    grid_node = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)[0]
    grid_node.set_node_position(-500, 0)
    try:
        s = grid_node.get_settings()
        s.set_editor_property("CoordinateSpace", 1) # RELATIVE
        s.set_editor_property("GridExtents", unreal.Vector(250, 250, 50))
        s.set_editor_property("CellSize", unreal.Vector(150, 150, 200))
    except: pass
    
    # Copy Points
    copy_node = graph.add_node_of_type(unreal.PCGCopyPointsSettings)[0]
    copy_node.set_node_position(-200, 0)
    
    # Lift
    lift_node = graph.add_node_of_type(unreal.PCGTransformPointsSettings)[0]
    lift_node.set_node_position(0, 0)
    try:
        s = lift_node.get_settings()
        v = unreal.Vector(0,0,2000)
        s.set_editor_property("OffsetMin", v)
        s.set_editor_property("OffsetMax", v)
        s.set_editor_property("bApplyTransform", True)
    except: pass
    
    # Projection
    proj_node = graph.add_node_of_type(unreal.PCGProjectionSettings)[0]
    proj_node.set_node_position(200, 0)
    try:
        s = proj_node.get_settings()
        tgt_val = unreal.PCGProjectionTarget.WORLD if hasattr(unreal.PCGProjectionTarget, 'WORLD') else 2
        s.set_editor_property("ProjectionTarget", tgt_val)
    except: pass
    
    # Distance
    dist_node = graph.add_node_of_type(unreal.PCGDistanceSettings)[0]
    dist_node.set_node_position(400, 0)
    try:
        s = dist_node.get_settings()
        s.set_editor_property("bSetDensity", True)
        s.set_editor_property("MaximumDistance", 3000.0)
    except: pass
    
    # Random Transform
    trans_node = graph.add_node_of_type(unreal.PCGTransformPointsSettings)[0]
    trans_node.set_node_position(600, 0)
    if rnd_trans_settings:
        try:
            ts = trans_node.get_settings()
            ts.set_editor_property("OffsetMin", rnd_trans_settings.get_editor_property("OffsetMin"))
            ts.set_editor_property("OffsetMax", rnd_trans_settings.get_editor_property("OffsetMax"))
            ts.set_editor_property("RotationMin", rnd_trans_settings.get_editor_property("RotationMin"))
            ts.set_editor_property("RotationMax", rnd_trans_settings.get_editor_property("RotationMax"))
            ts.set_editor_property("ScaleMin", rnd_trans_settings.get_editor_property("ScaleMin"))
            ts.set_editor_property("ScaleMax", rnd_trans_settings.get_editor_property("ScaleMax"))
        except: pass
        
    # 4. CONNECT
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Grid (Source) -> Copy (Source)
    connect(grid_node, copy_node, "Out", "Source")
    # Wall (Target) -> Copy (Target)
    if wall_node: connect(wall_node, copy_node, "Out", "Target")
    
    connect(copy_node, lift_node)
    connect(lift_node, proj_node)
    connect(proj_node, dist_node, "Out", "Source")
    
    if floor_node: connect(floor_node, dist_node, "Out", "Target")
    
    connect(dist_node, trans_node)
    for f in filters:
        connect(trans_node, f)
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Rebuild Complete.")
    
    # 5. SYNC (RefreshBlueprint)
    try:
        # We try to load UDungeonAssetUtils class
        # And call RefreshBlueprint(graph)
        # Assuming the library is exposed to Python as unreal.DungeonAssetUtils
        unreal.DungeonAssetUtils.refresh_blueprint(graph)
        print("Called RefreshBlueprint (Sync).")
    except Exception as e:
        print(f"Sync Warning: {e}")

"""

def clean_rebuild_sync():
    print(f"--- [Fix] Clean & Rebuild + Sync ---", flush=True)
    
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
    clean_rebuild_sync()
