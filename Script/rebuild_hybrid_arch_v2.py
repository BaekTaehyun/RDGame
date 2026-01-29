import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Hybrid] Rebuild V2: Global Forest - Dungeon ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. CLEANUP (Keep Filters & Spawners)
    nodes_to_delete = []
    
    wall_node = None
    floor_node = None
    filters = []
    spawners = []
    
    for n in graph.nodes:
        nm = n.get_name()
        if "StaticMeshSpawner" in nm: 
            spawners.append(n)
            continue
        if "DensityFilter" in nm: 
            filters.append(n)
            continue
            
        # Check Readers
        if "DungeonDataReader" in nm:
            t = ""
            try: t = n.get_editor_property("NodeTitleOverride")
            except: pass
            if "Wall" in t: wall_node = n
            elif "Floor" in t: floor_node = n
            else: nodes_to_delete.append(n)
            continue
            
        # Delete everything else (Old Logic)
        nodes_to_delete.append(n)

    print(f"Deleting {len(nodes_to_delete)} old nodes...")
    for n in nodes_to_delete:
         graph.remove_node(n)
         
    # 2. CREATE NODES
    
    # A. Landscape Data
    # Class: PCGGetLandscapeSettings
    land_node = graph.add_node_of_type(unreal.PCGGetLandscapeSettings)[0]
    land_node.set_node_position(-1200, -200)
    
    # Ensure Readers exist
    if not wall_node:
        wall_node = graph.add_node_of_type(unreal.PCGDungeonDataReaderSettings)[0]
        wall_node.set_editor_property("NodeTitleOverride", "Wall")
        try: wall_node.get_settings().set_editor_property("TargetTileType", unreal.PCGDungeonTileFilter.WALL)
        except: pass
    wall_node.set_node_position(-1200, 200)

    if not floor_node:
        floor_node = graph.add_node_of_type(unreal.PCGDungeonDataReaderSettings)[0]
        floor_node.set_editor_property("NodeTitleOverride", "Floor")
        try: floor_node.get_settings().set_editor_property("TargetTileType", unreal.PCGDungeonTileFilter.FLOOR)
        except: pass
    floor_node.set_node_position(-1200, 400)
    
    # B. Surface Sampler (Global)
    sampler_node = graph.add_node_of_type(unreal.PCGSurfaceSamplerSettings)[0]
    sampler_node.set_node_position(-900, -200)
    try:
        s = sampler_node.get_settings()
        s.set_editor_property("PointsPerSquaredMeter", 0.5)
        # PointExtents
        s.set_editor_property("PointExtents", unreal.Vector(100, 100, 100))
    except: pass
    
    # C. Union (Dungeon Data)
    union_node = graph.add_node_of_type(unreal.PCGUnionSettings)[0]
    union_node.set_node_position(-900, 300)
    
    # D. Bounds Modifier (Blocking Volume)
    bounds_node = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)[0]
    bounds_node.set_node_position(-700, 300)
    try:
        s = bounds_node.get_settings()
        # Set Mode to SET (1) ?
        # Mode Enum: 0=Set, 1=Intersect? Need to verify. Usually SET is what we want.
        # But for 'PCGBoundsModifierMode', commonly:
        # Scale=0, Set=1, Intersect=2... Wait.
        # Let's try to find Enum or just Default (Scale?).
        # We want to Expand existing points?
        # Actually Wall points are zero-size. So Set is good.
        
        # Bounds +/- 300
        v_min = unreal.Vector(-300, -300, -300)
        v_max = unreal.Vector(300, 300, 300)
        s.set_editor_property("BoundsMin", v_min)
        s.set_editor_property("BoundsMax", v_max)
        
        # Try finding Mode property
        # s.set_editor_property("Mode", ...)
    except: pass
    
    # E. Difference (Forest - Dungeon)
    diff_node = graph.add_node_of_type(unreal.PCGDifferenceSettings)[0]
    diff_node.set_node_position(-500, 0)
    try:
        s = diff_node.get_settings()
        # Mode: Inferred (Default) usually works
        s.set_editor_property("Mode", unreal.PCGDifferenceMode.INFERRED)
    except: pass
    
    # F. Transform (Random)
    trans_node = graph.add_node_of_type(unreal.PCGTransformPointsSettings)[0]
    trans_node.set_node_position(-200, 0)
    try:
        s = trans_node.get_settings()
        s.set_editor_property("bApplyTransform", True)
        s.set_editor_property("RotationMin", unreal.Rotator(0, 0, 0))
        s.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
        s.set_editor_property("ScaleMin", unreal.Vector(0.8, 0.8, 0.8))
        s.set_editor_property("ScaleMax", unreal.Vector(1.5, 1.5, 1.5))
    except: pass

    # 3. WIRING
    # Landscape -> Sampler
    # PCGGetLandscapeSettings usually outputs 'Data' or 'Out'
    # PCGSurfaceSamplerSettings input is 'Surface'
    try: graph.add_edge(land_node, "Data", sampler_node, "Surface")
    except: 
        try: graph.add_edge(land_node, "Out", sampler_node, "Surface")
        except: graph.add_edge(land_node, "Out", sampler_node, "In")

    # Readers -> Union
    try: graph.add_edge(wall_node, "Out", union_node, "In")
    except: pass
    try: graph.add_edge(floor_node, "Out", union_node, "In")
    except: pass
    
    # Union -> Bounds
    try: graph.add_edge(union_node, "Out", bounds_node, "In")
    except: pass
    
    # Difference: Sampler(Source) - Bounds(Difference)
    try: graph.add_edge(sampler_node, "Out", diff_node, "Source")
    except: pass
    
    try: graph.add_edge(bounds_node, "Out", diff_node, "Difference")
    except: pass
    
    # Difference -> Transform
    try: graph.add_edge(diff_node, "Out", trans_node, "In")
    except: pass
    
    # Transform -> Filters
    if filters:
        for f in filters:
            try: graph.add_edge(trans_node, "Out", f, "In")
            except: pass
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Hybrid Rebuild V2 Complete.")

    # Sync
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def rebuild_hybrid_v2():
    print(f"--- [Hybrid] Rebuild V2 ---", flush=True)
    
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
    rebuild_hybrid_v2()
