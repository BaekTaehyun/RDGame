import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Hybrid] Rebuild: Landscape - Dungeon ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Existing Resources (Preserve Spawners/Filters)
    spawners = []
    filters = []
    wall_node = None
    floor_node = None
    
    nodes_to_delete = []
    
    for n in graph.nodes:
        nm = n.get_name()
        # Keep Spawners & Filters
        if "StaticMeshSpawner" in nm: 
            spawners.append(n)
            continue
        if "DensityFilter" in nm: 
            filters.append(n)
            continue
            
        # Re-use Readers if present, else we recreate to be safe/clean
        # Actually, let's delete logic nodes to ensure clean graph
        if "DungeonDataReader" in nm:
            # Check title
            t = ""
            try: t = n.get_editor_property("NodeTitleOverride")
            except: pass
            if "Wall" in t: wall_node = n
            elif "Floor" in t: floor_node = n
            else: nodes_to_delete.append(n) # Delete untagged/confusing readers
            continue
        
        # Delete old logic (Grid, Copy, Transform, Proj, Distance, Difference, Union, Bounds)
        # We will rebuild them.
        nodes_to_delete.append(n)

    print(f"Deleting {len(nodes_to_delete)} old logic nodes...")
    for n in nodes_to_delete:
         graph.remove_node(n)
         
    # 2. CREATE NEW LOGIC NODES
    
    # A. Data Sources
    # Landscape Data
    land_node = graph.add_node_of_type(unreal.PCGGetLandscapeDataSettings)[0]
    land_node.set_node_position(-1000, -200)
    
    # Readers (Create if missing)
    if not wall_node:
        wall_node = graph.add_node_of_type(unreal.DungeonDataReaderSettings)[0]
        wall_node.set_editor_property("NodeTitleOverride", "Wall")
        # Set Type? Need to ensure it reads Wall (Enum 2)
        try: wall_node.get_settings().set_editor_property("TargetTileType", unreal.PCGDungeonTileFilter.WALL)
        except: pass
    wall_node.set_node_position(-1000, 200)

    if not floor_node:
        floor_node = graph.add_node_of_type(unreal.DungeonDataReaderSettings)[0]
        floor_node.set_editor_property("NodeTitleOverride", "Floor")
         # Set Type? Floor (Enum 1)
        try: floor_node.get_settings().set_editor_property("TargetTileType", unreal.PCGDungeonTileFilter.FLOOR)
        except: pass
    floor_node.set_node_position(-1000, 400)
    
    # B. Layer 1: Global Surface Sampler
    sampler_node = graph.add_node_of_type(unreal.PCGSurfaceSamplerSettings)[0]
    sampler_node.set_node_position(-700, -200)
    try:
        s = sampler_node.get_settings()
        s.set_editor_property("PointsPerSquaredMeter", 0.5) # Dense forest
        s.set_editor_property("PointExtents", unreal.Vector(100, 100, 100))
    except: pass
    
    # C. Layer 2: Exclusion (Blocking Volume)
    union_node = graph.add_node_of_type(unreal.PCGUnionSettings)[0]
    union_node.set_node_position(-700, 300)
    
    bounds_node = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)[0]
    bounds_node.set_node_position(-500, 300)
    try:
        s = bounds_node.get_settings()
        # Large bounds to block entrance/corridors fully
        v = unreal.Vector(300, 300, 300) 
        s.set_editor_property("BoundsMin", v.get_reversed())
        s.set_editor_property("BoundsMax", v)
        s.set_editor_property("Mode", unreal.PCGBoundsModifierMode.SET)
    except:
        # Fallback if get_reversed fails (it shouldn't in recent python, but manually:)
        try:
             s.set_editor_property("BoundsMin", unreal.Vector(-300,-300,-300))
             s.set_editor_property("BoundsMax", unreal.Vector(300,300,300))
        except: pass
        
    diff_node = graph.add_node_of_type(unreal.PCGDifferenceSettings)[0]
    diff_node.set_node_position(-300, 0)
    try:
        s = diff_node.get_settings()
        s.set_editor_property("Mode", unreal.PCGDifferenceMode.INFERRED) # or DISCRETE
    except: pass

    # D. Post-Process (Transform -> Filter)
    # We need a Transform Node to apply random rotation/scale to the forest
    trans_node = graph.add_node_of_type(unreal.PCGTransformPointsSettings)[0]
    trans_node.set_node_position(0, 0)
    try:
        s = trans_node.get_settings()
        s.set_editor_property("bApplyTransform", True)
        s.set_editor_property("RotationMin", unreal.Rotator(0, 0, 0))
        s.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
        s.set_editor_property("ScaleMin", unreal.Vector(0.8, 0.8, 0.8))
        s.set_editor_property("ScaleMax", unreal.Vector(1.5, 1.5, 1.5))
    except: pass

    # 3. WIRING
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Stream A: Landscape -> Sampler
    connect(land_node, sampler_node, "Data", "Surface") # Pin names might be 'Data'?'Surface'?
    # Actually GetLandscapeData output is usually 'Out' or 'Data'. SurfaceSampler input is 'Surface'.
    # Let's try blind 'Out' -> 'Surface' or 'In'.
    # Inspecting Sampler pins: Input is 'Surface'.
    # Landscape Data output: 'Out'.
    try: graph.add_edge(land_node, "Out", sampler_node, "Surface")
    except: graph.add_edge(land_node, "Out", sampler_node, "In") # Fallback

    # Stream B: Dungeon -> Union -> Bounds
    connect(wall_node, union_node, "Out", "In")
    connect(floor_node, union_node, "Out", "In")
    connect(union_node, bounds_node) # Out->In default
    
    # Difference: A - B
    # Sampler -> Source
    try: graph.add_edge(sampler_node, "Out", diff_node, "Source")
    except: pass
    
    # Bounds -> Target
    try: graph.add_edge(bounds_node, "Out", diff_node, "Difference") # Pin name is 'Difference' or 'Target'?
    # According to docs/experience, Difference node has 'Source' and 'Difference' (Subtraction).
    except: pass
    
    # Result -> Transform
    connect(diff_node, trans_node)
    
    # Transform -> Filters (Connecting to ALL existing filters for now)
    # We will let the filters handle density.
    if filters:
        for f in filters:
            connect(trans_node, f)
    else:
        # If no filters found (unlikely), create a debug output?
        pass

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Hybrid Rebuild Complete.")
    
    # Sync
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def rebuild_hybrid():
    print(f"--- [Hybrid] Rebuild ---", flush=True)
    
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
    rebuild_hybrid()
