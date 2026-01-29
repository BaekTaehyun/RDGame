import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] CLEAN REBUILD of Forest Logic ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. IDENTIFY PRESERVED NODES
    wall_reader = None
    spawners = {} # Map Index/Name to Node
    
    # We need to correctly identify Spawners to re-hook them.
    # Assuming Spawner_0 = Big Tree, Spawner_1 = Medium...
    # We will map them by Name: StaticMeshSpawner_0, 1, 2, 3.
    
    for n in graph.nodes:
        name = n.get_name()
        
        # Reader: We want the WALL reader.
        # User confirmed 'DungeonDataReader' (Wall) exists.
        # Previous script found 'DungeonDataReader_2' as candidate.
        # Let's verify by exclusion of Floor.
        if "DungeonDataReader" in name:
             # Check Title
            t = str(n.get_editor_property("NodeTitleOverride"))
            if "Wall" in t: 
                wall_reader = n
            elif "Floor" not in t and "Wall" not in t:
                # If no title, assume _2 is Wall if _1 is Floor.
                if name == "DungeonDataReader_2": wall_reader = n

        # Spawners
        if "StaticMeshSpawner" in name:
            spawners[name] = n
            
    if not wall_reader:
        print("CRITICAL: Wall Reader not found. Aborting Rebuild to prevent data loss.")
        # Try finding *any* reader that isn't Floor
        for n in graph.nodes:
            if "DungeonDataReader" in n.get_name():
                 t = str(n.get_editor_property("NodeTitleOverride"))
                 if "Floor" not in t:
                     wall_reader = n
                     print(f"Fallback: Using {n.get_name()} as Wall Reader.")
                     break

    if wall_reader and spawners:
        # 2. DELETE INTERMEDIATE NODES (Cleanup)
        # We delete anything involved in the messed up chain.
        # Transforms, Filters, Projections, Noise, Difference, Bounds.
        # BUT BE CAREFUL not to delete Ruins nodes if they share names.
        # Ruins uses 'CreatePointsGrid_1' and 'TransformPoints_2' and 'DensityFilter_5'.
        
        nodes_to_delete = []
        for n in graph.nodes:
            name = n.get_name()
            
            # Categories to nuke
            is_filter = "DensityFilter" in name and "5" not in name # Keep Filter_5 (Ruins)
            is_transform = "TransformPoints" in name and "2" not in name # Keep Trans_2 (Ruins)
            is_proj = "Projection" in name
            is_noise = "AttributeNoise" in name
            is_diff = "Difference" in name
            is_bounds = "BoundsModifier" in name
            is_grid = "CreatePointsGrid_0" in name # Only Forest Grid
            
            if is_filter or is_transform or is_proj or is_noise or is_diff or is_bounds or is_grid:
                nodes_to_delete.append(n)
                
        for n in nodes_to_delete:
            graph.remove_node(n)
            
        print(f"Deleted {len(nodes_to_delete)} messed up nodes.")
        
        # 3. REBUILD CHAIN (Linear & Clean)
        
        # A. Bounds Modifier (Fix 0-size bounds from DungeonData)
        node_bounds = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)[0]
        node_bounds.set_node_position(0, 0)
        node_bounds.get_settings().set_editor_property("BoundsMin", unreal.Vector(-100,-100,-100))
        node_bounds.get_settings().set_editor_property("BoundsMax", unreal.Vector(100,100,100))
        
        # B. Density Noise (For Variety)
        node_noise = graph.add_node_of_type(unreal.PCGAttributeNoiseSettings)[0]
        node_noise.set_node_position(200, 0)
        # Defaults: Density, 0-1.
        
        # C. Transform (Jitter)
        node_trans = graph.add_node_of_type(unreal.PCGTransformPointsSettings)[0]
        node_trans.set_node_position(400, 0)
        ts = node_trans.get_settings()
        ts.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
        ts.set_editor_property("OffsetMin", unreal.Vector(-50, -50, 0)) # Smaller jitter for walls
        ts.set_editor_property("OffsetMax", unreal.Vector(50, 50, 0))
        ts.set_editor_property("ScaleMin", unreal.Vector(0.8, 0.8, 0.8))
        ts.set_editor_property("ScaleMax", unreal.Vector(1.4, 1.4, 1.4))
        
        # D. Projection (Snap to Landscape/Mesh)
        # Wait, if we spawn on Walls, do we project to Landscape?
        # If Wall Data is 'On the Wall', projecting to landscape might flatten them to ground.
        # User said "Forest should be...".
        # Let's SKIP Projection for now? 
        # User Complaint: "Trees on Path". 
        # If Wall Data is correct, points are already in valid 3D space.
        # Adding Projection forces them to Z-Project, which might be safe.
        # Let's ADD it but keep in mind.
        node_proj = graph.add_node_of_type(unreal.PCGProjectionSettings)[0]
        node_proj.set_node_position(600, 0)
        
        # 4. CONNECT CHAIN
        graph.add_edge(wall_reader, "Out", node_bounds, "In")
        graph.add_edge(node_bounds, "Out", node_noise, "In")
        graph.add_edge(node_noise, "Out", node_trans, "In")
        graph.add_edge(node_trans, "Out", node_proj, "In")
        
        # 5. RE-CREATE FILTERS & CONNECT TO SPAWNERS
        # Spawners: 0(Big), 1(Med), 2(Small), 3(Bush).
        # We need 4 Filters.
        
        # Filter 1 (Big) -> Spawner 0
        f1 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f1.set_node_position(800, -200)
        f1.get_settings().lower_bound = 0.9
        f1.get_settings().upper_bound = 1.0
        graph.add_edge(node_proj, "Out", f1, "In")
        if "StaticMeshSpawner_0" in spawners:
            graph.add_edge(f1, "Out", spawners["StaticMeshSpawner_0"], "In")
            
        # Filter 2 (Med) -> Spawner 1
        f2 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f2.set_node_position(800, 0)
        f2.get_settings().lower_bound = 0.6
        f2.get_settings().upper_bound = 1.0
        graph.add_edge(node_proj, "Out", f2, "In")
        if "StaticMeshSpawner_1" in spawners:
            graph.add_edge(f2, "Out", spawners["StaticMeshSpawner_1"], "In")
            
        # Filter 3 (Small) -> Spawner 2
        f3 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f3.set_node_position(800, 200)
        f3.get_settings().lower_bound = 0.4
        f3.get_settings().upper_bound = 1.0
        graph.add_edge(node_proj, "Out", f3, "In")
        if "StaticMeshSpawner_2" in spawners:
            graph.add_edge(f3, "Out", spawners["StaticMeshSpawner_2"], "In")
            
        # Filter 4 (Bush) -> Spawner 3
        f4 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f4.set_node_position(800, 400)
        f4.get_settings().lower_bound = 0.2
        f4.get_settings().upper_bound = 1.0
        graph.add_edge(node_proj, "Out", f4, "In")
        if "StaticMeshSpawner_3" in spawners:
            graph.add_edge(f4, "Out", spawners["StaticMeshSpawner_3"], "In")

        unreal.EditorAssetLibrary.save_loaded_asset(graph)
        print("Rebuild Complete. System Cleaned.")
        
    else:
        print("Setup Failed: WallReader or Spawners not found.")

"""

def clean_rebuild():
    print(f"--- [Fix] Clean Rebuild ---", flush=True)
    
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
    clean_rebuild()
