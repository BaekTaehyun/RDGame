import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] CLEAN REBUILD v2 (Robust) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. IDENTIFY PRESERVED NODES
    wall_reader = None
    spawners = {} 
    
    for n in graph.nodes:
        name = n.get_name()
        
        # Reader Identification (Robust)
        # Try Title first, Fallback to Name
        title = ""
        try: title = str(n.get_editor_property("NodeTitleOverride"))
        except: pass
        
        if "DungeonDataReader" in name:
            if "Wall" in title:
                wall_reader = n
                print(f"Identified Wall Reader by Title: {name}")
            elif "Floor" in title:
                pass # Explicitly ignore Floor
            elif name == "DungeonDataReader_2": 
                # Fallback: User feedback implies _2 is Wall if _1 is Floor
                if not wall_reader:
                     wall_reader = n
                     print(f"Identified Wall Reader by Name: {name}")

        # Spawners
        if "StaticMeshSpawner" in name:
            spawners[name] = n
            
    if not wall_reader:
        print("CRITICAL: Wall Reader not found. Start Over?")
        # Last ditch: Find ANY reader that isn't _1 (Floor)
        for n in graph.nodes:
             if "DungeonDataReader" in n.get_name() and n.get_name() != "DungeonDataReader_1":
                 wall_reader = n
                 print(f"Emergency Fallback: Using {n.get_name()}")
                 break

    if wall_reader and spawners:
        print(f"Ref: Wall={wall_reader.get_name()}, Spawners={len(spawners)}")
        
        # 2. DELETE INTERMEDIATE NODES
        nodes_to_delete = []
        for n in graph.nodes:
            name = n.get_name()
            
            # Delete categories
            # Note: We keep Ruins (Filter_5, Trans_2, Grid_1)
            is_filter = "DensityFilter" in name and "5" not in name
            is_transform = "TransformPoints" in name and "2" not in name
            is_proj = "Projection" in name
            is_noise = "AttributeNoise" in name
            is_diff = "Difference" in name
            is_bounds = "BoundsModifier" in name
            is_grid = "CreatePointsGrid" in name and "1" not in name # Grid 0 or others
            
            if is_filter or is_transform or is_proj or is_noise or is_diff or is_bounds or is_grid:
                nodes_to_delete.append(n)
                
        for n in nodes_to_delete:
            try: graph.remove_node(n)
            except: pass
            
        print(f"Deleted {len(nodes_to_delete)} intermediate nodes.")
        
        # 3. REBUILD CHAIN (Linear & Clean)
        try:
            # A. Bounds Modifier
            ret = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)
            node_bounds = ret[0]
            node_bounds.set_node_position(0, 0)
            try:
                node_bounds.get_settings().set_editor_property("BoundsMin", unreal.Vector(-100,-100,-100))
                node_bounds.get_settings().set_editor_property("BoundsMax", unreal.Vector(100,100,100))
            except: pass
            
            # B. Density Noise
            ret = graph.add_node_of_type(unreal.PCGAttributeNoiseSettings)
            node_noise = ret[0]
            node_noise.set_node_position(200, 0)
            
            # C. Transform
            ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
            node_trans = ret[0]
            node_trans.set_node_position(400, 0)
            try:
                ts = node_trans.get_settings()
                ts.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
                ts.set_editor_property("OffsetMin", unreal.Vector(-50, -50, 0))
                ts.set_editor_property("OffsetMax", unreal.Vector(50, 50, 0))
                ts.set_editor_property("ScaleMin", unreal.Vector(0.8, 0.8, 0.8))
                ts.set_editor_property("ScaleMax", unreal.Vector(1.4, 1.4, 1.4))
            except: pass
            
            # D. Projection
            ret = graph.add_node_of_type(unreal.PCGProjectionSettings)
            node_proj = ret[0]
            node_proj.set_node_position(600, 0)
            
            # 4. CONNECT CHAIN
            graph.add_edge(wall_reader, "Out", node_bounds, "In")
            graph.add_edge(node_bounds, "Out", node_noise, "In")
            graph.add_edge(node_noise, "Out", node_trans, "In")
            graph.add_edge(node_trans, "Out", node_proj, "In")
            
            # 5. RE-CREATE FILTERS & CONNECT
            # Filter 1 (Big)
            f1 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
            f1.set_node_position(800, -200)
            try:
                f1.get_settings().lower_bound = 0.9
                f1.get_settings().upper_bound = 1.0
            except: pass
            graph.add_edge(node_proj, "Out", f1, "In")
            if "StaticMeshSpawner_0" in spawners:
                # IMPORTANT: Spawners inputs may be occupied? 
                # graph.add_edge usually works (PCG allows Multiple Inputs).
                # But we want to ensure it's the ONLY input?
                # We can't break previous links easily if they exist ( Ruin Spawner? No, these are forest spawners).
                # We deleted the previous nodes feeding them, so they should be free!
                graph.add_edge(f1, "Out", spawners["StaticMeshSpawner_0"], "In")
                
            # Filter 2 (Med)
            f2 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
            f2.set_node_position(800, 0)
            try: f2.get_settings().lower_bound = 0.6
            except: pass
            graph.add_edge(node_proj, "Out", f2, "In")
            if "StaticMeshSpawner_1" in spawners:
                graph.add_edge(f2, "Out", spawners["StaticMeshSpawner_1"], "In")
                
            # Filter 3 (Small)
            f3 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
            f3.set_node_position(800, 200)
            try: f3.get_settings().lower_bound = 0.4
            except: pass
            graph.add_edge(node_proj, "Out", f3, "In")
            if "StaticMeshSpawner_2" in spawners:
                graph.add_edge(f3, "Out", spawners["StaticMeshSpawner_2"], "In")
                
            # Filter 4 (Bush/Ground)
            f4 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
            f4.set_node_position(800, 400)
            try: f4.get_settings().lower_bound = 0.2
            except: pass
            graph.add_edge(node_proj, "Out", f4, "In")
            if "StaticMeshSpawner_3" in spawners:
                graph.add_edge(f4, "Out", spawners["StaticMeshSpawner_3"], "In")
                
            print("Rebuild Complete. Linear Chain Established.")
            
        except Exception as e:
            print(f"Rebuild Error: {e}")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved.")

"""

def clean_rebuild_v2():
    print(f"--- [Fix] Clean Rebuild v2 ---", flush=True)
    
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
    clean_rebuild_v2()
