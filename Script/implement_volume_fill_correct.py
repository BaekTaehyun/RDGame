import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Volume Filling (Correction) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Helpers
    wall_reader = None
    floor_reader = None
    spawners = {}
    
    for n in graph.nodes:
        name = n.get_name()
        if "DungeonDataReader" in name and "2" in name: wall_reader = n
        t = ""
        try: t = str(n.get_editor_property("NodeTitleOverride"))
        except: pass
        if "Floor" in t: floor_reader = n
        elif not floor_reader and "DungeonDataReader" in name and n != wall_reader:
             floor_reader = n
        if "StaticMeshSpawner" in name: spawners[name] = n
        
    if wall_reader and floor_reader:
        # 2. CLEAR INTERMEDIATE NODES
        nodes_to_del = []
        for n in graph.nodes:
            nm = n.get_name()
            # Keep Readers and Spawners.
            if "DungeonDataReader" not in nm and "StaticMeshSpawner" not in nm and "GraphInput" not in nm and "Output" not in nm:
                 nodes_to_del.append(n)
        
        for n in nodes_to_del:
            try: graph.remove_node(n)
            except: pass
            
        print("Cleared Chain.")

        # 3. CONSTRUCT CHAIN
        
        # A. Bounds (On Wall Points) - 200 extend
        ret = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)
        bounds = ret[0]
        bounds.set_node_position(200, 0)
        try:
            sz = 200.0
            bounds.get_settings().set_editor_property("BoundsMin", unreal.Vector(-sz,-sz,-sz))
            bounds.get_settings().set_editor_property("BoundsMax", unreal.Vector(sz,sz,sz))
        except: pass
        
        # B. Create Points Grid (Volume Filler)
        ret = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
        grid = ret[0]
        grid.set_node_position(400, 0)
        try:
            # Denser grid: 80
            grid.get_settings().set_editor_property("CellSize", unreal.Vector(80, 80, 100))
        except: pass
        
        # C. Distance (Ecotone)
        ret = graph.add_node_of_type(unreal.PCGDistanceSettings)
        dist_node = ret[0]
        dist_node.set_node_position(600, 0)
        try:
            dist_node.get_settings().set_editor_property("bSetDensity", True)
            dist_node.get_settings().set_editor_property("MaximumDistance", 1500.0) 
        except: pass

        # D. Transform (Jitter)
        ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
        trans = ret[0]
        trans.set_node_position(800, 0)
        try:
            trans.get_settings().set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
            trans.get_settings().set_editor_property("OffsetMin", unreal.Vector(-30, -30, 0))
            trans.get_settings().set_editor_property("OffsetMax", unreal.Vector(30, 30, 0))
            trans.get_settings().set_editor_property("ScaleMin", unreal.Vector(0.7, 0.7, 0.7))
            trans.get_settings().set_editor_property("ScaleMax", unreal.Vector(1.3, 1.3, 1.3))
        except: pass
        
        # E. Projection (Snap)
        ret = graph.add_node_of_type(unreal.PCGProjectionSettings)
        proj = ret[0]
        proj.set_node_position(1000, 0) # Project before filters?
        
        # 4. WIRE IT UP
        
        graph.add_edge(wall_reader, "Out", bounds, "In")
        graph.add_edge(bounds, "Out", grid, "In")
        
        # Grid -> Distance (Source)
        graph.add_edge(grid, "Out", dist_node, "Source")
        # Floor -> Distance (Target)
        graph.add_edge(floor_reader, "Out", dist_node, "Target")
            
        # Distance -> Transform
        graph.add_edge(dist_node, "Out", trans, "In")
        graph.add_edge(trans, "Out", proj, "In")
        
        # 5. FILTERS
        # F1 (High Density / Close to Path?) -> Big/Small?
        # Typically Distance 0 (Path) -> Density 1? Or Density 0?
        # If SetDensity sets 1 at Source and 0 at MaxDistance? Or 0 at Source?
        # Standard: 0 at Source, 1 at Target? No.
        # Let's assume Filter 0.8-1.0 captures one end.
        
        # F1
        f1 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f1.set_node_position(1200, -200)
        f1.get_settings().lower_bound = 0.8
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
        f3.get_settings().lower_bound = 0.2
        graph.add_edge(proj, "Out", f3, "In")
        if "StaticMeshSpawner_2" in spawners: graph.add_edge(f3, "Out", spawners["StaticMeshSpawner_2"], "In")
        
        # F4
        f4 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f4.set_node_position(1200, 400)
        f4.get_settings().lower_bound = 0.0
        f4.get_settings().upper_bound = 0.2
        graph.add_edge(proj, "Out", f4, "In")
        if "StaticMeshSpawner_3" in spawners: graph.add_edge(f4, "Out", spawners["StaticMeshSpawner_3"], "In")
        
        print("Logic Rebuilt: Volume Fill + Ecotone Distance.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Done.")

"""

def implement_ecotone_fix():
    print(f"--- [Fix] Ecotone Fix ---", flush=True)
    
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
    implement_ecotone_fix()
