import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
paths = [
    "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged",
    "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar.SM_Stone_Pillar",
    "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Column_Destroyed.SM_Stone_Column_Destroyed"
]

print("--- [EMERGENCY] Recreating Ruins Spawner (Prop Fix) ---")

graph = unreal.load_asset(graph_path)
if not graph:
    print("Error: Graph not found!")
else:
    # 1. Cleanup Old Nodes
    to_delete = []
    parent_node = None # SelfPruning
    
    for n in graph.nodes:
        name = n.get_name()
        if name == "SelfPruning_0":
            parent_node = n
        
        # Identify our previous attempts
        if name in ["StaticMeshSpawner_4", "DensityFilter_0", "TransformPoints_0", "Ruins_Filter", "Ruins_Variator", "Ruins_Spawner", "Ruins_Filter_Fix", "Ruins_Variator_Fix", "Ruins_Spawner_Fix", "Ruins_Spawner_Final", "Ruins_Variator_Final", "Ruins_Filter_Final"]:
            to_delete.append(n)
            
    # Delete info
    if to_delete:
        print(f"Cleaning up {len(to_delete)} old nodes...")
        for n in to_delete:
            graph.remove_node(n)

    # Modify Graph for Undo/Redo transaction support
    graph.modify()
    
    # 2. Create New Chain
    base_x = 656
    base_y = 1200 

    # Filter
    ret = graph.add_node_of_type(unreal.PCGDensityFilterSettings)
    filter_node = ret[0]
    
    try:
        filter_node.set_editor_property("NodeTitleOverride", "Ruins_Filter_Final")
        # Position might be tricky, usually it is 'PositionX' or similar in Editor properties
        # But let's try standard python attr first, if fails we catch it.
        filter_node.position_x = base_x + 300
        filter_node.position_y = base_y
    except:
        pass # Ignore position error to ensure creation at least
    
    # Configure Filter
    filter_settings = filter_node.get_settings()
    filter_settings.lower_bound = 0.05
    filter_settings.upper_bound = 0.3

    # Transform
    ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
    trans_node = ret[0]
    try:
        trans_node.set_editor_property("NodeTitleOverride", "Ruins_Variator_Final")
        trans_node.position_x = base_x + 600
        trans_node.position_y = base_y
    except: pass
    
    # Spawner
    ret = graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
    spawner_node = ret[0]
    try:
        spawner_node.set_editor_property("NodeTitleOverride", "Ruins_Spawner_Final") 
        spawner_node.position_x = base_x + 900
        spawner_node.position_y = base_y
    except: pass
    
    # Configure Spawner Meshes
    spawner_settings = spawner_node.get_settings()
    selector = spawner_settings.mesh_selector_parameters
    
    try:
        entries = []
        for p in paths:
            mesh_obj = unreal.load_asset(p)
            if mesh_obj:
                entry = unreal.PCGMeshSelectorWeightedEntry()
                desc = entry.get_editor_property("Descriptor")
                desc.set_editor_property("StaticMesh", mesh_obj)
                entry.set_editor_property("Descriptor", desc)
                entry.set_editor_property("Weight", 1)
                entries.append(entry)
        
        selector.set_editor_property("MeshEntries", entries)
        print(f"Assigned {len(entries)} meshes to Spawner.")
    except Exception as e:
        print(f"Mesh assignment error: {e}")

    # 3. Connect
    try:
        if parent_node:
             graph.add_edge(parent_node, "Out", filter_node, "In")
             
        graph.add_edge(filter_node, "Out", trans_node, "In")
        graph.add_edge(trans_node, "Out", spawner_node, "In")
        print("Connected Chain Successfully.")
        
    except Exception as e:
         print(f"Connection error: {e}")

    # 4. Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("--- RECREATION COMPLETE ---")
    print(f"Created: {filter_node.get_name()}, {trans_node.get_name()}, {spawner_node.get_name()}")
"""

def force_recreate():
    print(f"--- [Fix] Running Emergency Recreate ---", flush=True)
    
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
    force_recreate()
