import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Volume Filling (Density) & Ecotone (Distance) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Helpers
    wall_reader = None
    floor_reader = None # Need this for distance target
    spawners = {}
    
    # Try to reuse existing nodes to avoid duplication
    for n in graph.nodes:
        name = n.get_name()
        # Wall Reader (We assume _2 based on history)
        if "DungeonDataReader" in name and "2" in name: wall_reader = n
        
        # Floor Reader (We assume _1 or look for Title)
        t = ""
        try: t = str(n.get_editor_property("NodeTitleOverride"))
        except: pass
        if "Floor" in t: floor_reader = n
        elif not floor_reader and "DungeonDataReader" in name and n != wall_reader:
             floor_reader = n
             
        if "StaticMeshSpawner" in name: spawners[name] = n
        
    print(f"Wall: {wall_reader.get_name() if wall_reader else 'Missing'}")
    print(f"Floor: {floor_reader.get_name() if floor_reader else 'Missing'}")

    if wall_reader and floor_reader:
        # 2. CLEAR INTERMEDIATE NODES (Again, to be safe and insert Grid)
        # We need to insert Grid between Bounds and Transform.
        # It's easier to nuke the chain and rebuild.
        
        nodes_to_del = []
        for n in graph.nodes:
            nm = n.get_name()
            # Keep Readers and Spawners. Delete everything else.
            if "DungeonDataReader" not in nm and "StaticMeshSpawner" not in nm and "GraphInput" not in nm and "Output" not in nm:
                 nodes_to_del.append(n)
        
        for n in nodes_to_del:
            try: graph.remove_node(n)
            except: pass
            
        print("Cleared Chain for Insertion.")

        # 3. CONSTRUCT CHAIN
        
        # A. Bounds (On Wall Points) - Make them BIG volumes to fill
        ret = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)
        bounds = ret[0]
        bounds.set_node_position(200, 0)
        try:
            # Volume size: 400x400x400 (Cover the tile)
            sz = 200.0
            bounds.get_settings().set_editor_property("BoundsMin", unreal.Vector(-sz,-sz,-sz))
            bounds.get_settings().set_editor_property("BoundsMax", unreal.Vector(sz,sz,sz))
            # Mode = Set
        except: pass
        
        # B. Create Points Grid (The Volume Filler)
        ret = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
        grid = ret[0]
        grid.set_node_position(400, 0)
        try:
            # Cell Size (Tree Spacing)
            grid.get_settings().set_editor_property("CellSize", unreal.Vector(60, 60, 100))
            # Cull Points Outside Operation? Default is usually "Inside Bounds".
            # We hope connection behavior "Input -> Grid" triggers "Spawn in Bounds".
        except: pass
        
        # C. Distance (Ecotone Logic)
        ret = graph.add_node_of_type(unreal.PCGDistanceSettings) # Verify class name?
        # Usually PCGDistanceSettings works? Or it might be an Attribute op?
        # Checked API before, it exists.
        dist_node = ret[0] if ret else None
        if dist_node:
            dist_node.set_node_position(600, 0)
            try:
                # SetDensity = True
                dist_node.get_settings().set_editor_property("bSetDensity", True)
                dist_node.get_settings().set_editor_property("MaximumDistance", 2000.0) # Falloff range
            except: pass
        else:
            print("Warning: Failed to create Distance Node.")

        # D. Transform (Jitter)
        ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
        trans = ret[0]
        trans.set_node_position(800, 0)
        try:
            trans.get_settings().set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
            trans.get_settings().set_editor_property("OffsetMin", unreal.Vector(-30, -30, 0))
            trans.get_settings().set_editor_property("OffsetMax", unreal.Vector(30, 30, 0))
        except: pass
        
        # E. Projection (Snap)
        ret = graph.add_node_of_type(unreal.PCGProjectionSettings)
        proj = ret[0]
        proj.set_node_position(1000, 0)
        
        # 4. WIRE IT UP
        
        # Wall -> Bounds
        graph.add_edge(wall_reader, "Out", bounds, "In")
        
        # Bounds -> Grid (Input)
        # Assuming Grid accepts Input to spawn inside it.
        # If not, we might need 'Volume Sampler' or similar.
        # PCG standard: Grid has input pin. If connected, it bounds generation.
        graph.add_edge(bounds, "Out", grid, "In")
        
        if dist_node:
            # Grid -> Distance (Source)
            graph.add_edge_by_name(grid, "Out", dist_node, "Source")
            # Floor -> Distance (Target)
            graph.add_edge_by_name(floor_reader, "Out", dist_node, "Target")
            
            # Distance -> Transform
            graph.add_edge(dist_node, "Out", trans, "In")
        else:
            graph.add_edge(grid, "Out", trans, "In")
            
        graph.add_edge(trans, "Out", proj, "In")
        
        # 5. FILTERS (Based on Density from Distance)
        # Distance Node usually sets Density: Close=1, Far=0? Or Close=0, Far=1?
        # Usually Close=1 (High Density).
        # We want: 
        #   Close to Path (Distance 0) -> No Trees? Or Small?
        #   Far from Path -> Big Trees?
        # If Distance Sets Density 0..1 (Normalized), we filter on that.
        
        # Let's create Filters assuming Density implies "Can Spawn".
        # We need range checks.
        
        # Filter 1 (Big Trees - Far?) -> Density 0.0 - 0.3 (If 0 is Far?)
        # Filter 2 (Small Trees - Close?) -> Density 0.7 - 1.0?
        # We'll just restore tiered filters and user can tune.
        
        # F1 -> Spawner 0
        f1 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f1.set_node_position(1200, -200)
        f1.get_settings().lower_bound = 0.8 # Very Dense/Close?
        graph.add_edge(proj, "Out", f1, "In")
        if "StaticMeshSpawner_0" in spawners: graph.add_edge(f1, "Out", spawners["StaticMeshSpawner_0"], "In")

        # F2
        f2 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f2.set_node_position(1200, 0)
        f2.get_settings().lower_bound = 0.5
        graph.add_edge(proj, "Out", f2, "In")
        if "StaticMeshSpawner_1" in spawners: graph.add_edge(f2, "Out", spawners["StaticMeshSpawner_1"], "In")
        
        # F3
        f3 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f3.set_node_position(1200, 200)
        f3.get_settings().lower_bound = 0.3
        graph.add_edge(proj, "Out", f3, "In")
        if "StaticMeshSpawner_2" in spawners: graph.add_edge(f3, "Out", spawners["StaticMeshSpawner_2"], "In")
        
        # F4
        f4 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f4.set_node_position(1200, 400)
        f4.get_settings().lower_bound = 0.1
        graph.add_edge(proj, "Out", f4, "In")
        if "StaticMeshSpawner_3" in spawners: graph.add_edge(f4, "Out", spawners["StaticMeshSpawner_3"], "In")
        
        print("Logic Rebuilt: Wall->Bounds->Grid->Distance(vs Floor)->Transform->Project->Filters.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Done.")

"""

def implement_ecotone():
    print(f"--- [Fix] Ecotone Logic ---", flush=True)
    
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
    implement_ecotone()
