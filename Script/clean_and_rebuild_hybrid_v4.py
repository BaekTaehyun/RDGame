import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Hybrid] Rebuild V4: Crash Fix & Complete Build ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. CLEANUP (Keep Filters & Spawners, Delete REST)
    nodes_to_delete = []
    
    # We will identify Spawners/Filters by their Settings Class
    # Everything else is considered "Logic" to be rebuilt.
    
    filter_nodes = []
    
    for n in graph.nodes:
        is_valuable = False
        settings = None
        try: settings = n.get_settings()
        except: pass
        
        if settings:
            s_class = settings.get_class().get_name()
            
            # Keep Spawners
            if "StaticMeshSpawner" in s_class:
                is_valuable = True
                
            # Keep Filters (track them for rewiring)
            elif "DensityFilter" in s_class:
                is_valuable = True
                filter_nodes.append(n)
                
            # Keep Input/Output
            elif "GraphInput" in s_class or "GraphOutput" in s_class:
                is_valuable = True
                
        # If not valuable, mark for death
        if not is_valuable:
            print(f"Marking for delete: {n.get_name()}")
            nodes_to_delete.append(n)

    print(f"Deleting {len(nodes_to_delete)} logic nodes...")
    for n in nodes_to_delete:
         graph.remove_node(n)

    # 2. CREATE NODES
    # A. Landscape -> Sampler (Layer 1)
    land_node = graph.add_node_of_type(unreal.PCGGetLandscapeSettings)[0]
    land_node.set_node_position(-1200, -200)
    
    sampler_node = graph.add_node_of_type(unreal.PCGSurfaceSamplerSettings)[0]
    sampler_node.set_node_position(-900, -200)
    try:
        s = sampler_node.get_settings()
        s.set_editor_property("PointsPerSquaredMeter", 0.5)
        s.set_editor_property("PointExtents", unreal.Vector(100, 100, 100))
    except: pass
    
    # B. Dungeon Logic (Layer 2)
    # Readers
    wall_node = graph.add_node_of_type(unreal.PCGDungeonDataReaderSettings)[0]
    try: wall_node.set_editor_property("NodeTitleOverride", "Wall")
    except: pass
    try: wall_node.get_settings().set_editor_property("TargetTileType", unreal.PCGDungeonTileFilter.WALL)
    except: pass
    wall_node.set_node_position(-1200, 200)

    floor_node = graph.add_node_of_type(unreal.PCGDungeonDataReaderSettings)[0]
    try: floor_node.set_editor_property("NodeTitleOverride", "Floor")
    except: pass
    try: floor_node.get_settings().set_editor_property("TargetTileType", unreal.PCGDungeonTileFilter.FLOOR)
    except: pass
    floor_node.set_node_position(-1200, 400)
    
    # Union
    union_node = graph.add_node_of_type(unreal.PCGUnionSettings)[0]
    union_node.set_node_position(-900, 300)
    
    # Bounds (Blocking Volume)
    bounds_node = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)[0]
    bounds_node.set_node_position(-700, 300)
    try:
        s = bounds_node.get_settings()
        v_min = unreal.Vector(-300, -300, -300)
        v_max = unreal.Vector(300, 300, 300)
        s.set_editor_property("BoundsMin", v_min)
        s.set_editor_property("BoundsMax", v_max)
        # Mode Set
        try: s.set_editor_property("Mode", unreal.PCGBoundsModifierMode.SET)
        except: pass
    except: pass
    
    # Difference
    diff_node = graph.add_node_of_type(unreal.PCGDifferenceSettings)[0]
    diff_node.set_node_position(-500, 0)
    try:
        s = diff_node.get_settings()
        s.set_editor_property("Mode", unreal.PCGDifferenceMode.INFERRED)
    except: pass
    
    # Transform
    trans_node = graph.add_node_of_type(unreal.PCGTransformPointsSettings)[0]
    trans_node.set_node_position(-200, 0)
    try:
        s = trans_node.get_settings()
        s.set_editor_property("bApplyTransform", True)
        s.set_editor_property("RotationMin", unreal.Rotator(0, 0, 0))
        s.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
    except: pass
    
    # 3. WIRING
    def connect_safe(src, dst, sp="Out", dp="In"):
        if not src or not dst: return
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Landscape -> Sampler
    try: graph.add_edge(land_node, "Data", sampler_node, "Surface")
    except: 
        try: graph.add_edge(land_node, "Out", sampler_node, "Surface")
        except: connect_safe(land_node, sampler_node, "Out", "In")
        
    # Readers -> Union
    connect_safe(wall_node, union_node, "Out", "In")
    connect_safe(floor_node, union_node, "Out", "In")
    
    # Union -> Bounds
    connect_safe(union_node, bounds_node)
    
    # Difference: Sampler(Source) - Bounds(Difference)
    # Sampler -> Source
    try: graph.add_edge(sampler_node, "Out", diff_node, "Source")
    except: pass
    
    # Bounds -> Difference
    # Note: Pin name 'Difference' is key. Or 'Target'.
    try: graph.add_edge(bounds_node, "Out", diff_node, "Difference")
    except: 
        try: graph.add_edge(bounds_node, "Out", diff_node, "Target")
        except: pass

    # Result -> Transform
    connect_safe(diff_node, trans_node)
    
    # Transform -> Existing Filters
    for f in filter_nodes:
        connect_safe(trans_node, f)

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Hybrid Rebuild V4 Complete.")
    
    # Sync
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def rebuild_hybrid_v4():
    print(f"--- [Hybrid] Rebuild V4 ---", flush=True)
    
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
    rebuild_hybrid_v4()
