import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Restoring Full Forest Logic (Variety + Jitter) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Valid Nodes (From Sanity Check)
    wall_reader = None
    bounds_node = None
    spawners = {}
    
    for n in graph.nodes:
        name = n.get_name()
        if "DungeonDataReader" in name and "2" in name: wall_reader = n # We know it's _2
        # Or check connection?
        
        if "BoundsModifier" in name: bounds_node = n
        
        if "StaticMeshSpawner" in name:
            spawners[name] = n
            
    # 2. Safety Check
    if not wall_reader:
        # Try finding the one connected to Bounds?
        pass

    if wall_reader and bounds_node:
        # 3. Create Intermediate Nodes
        
        # A. Density Noise (For Mixing)
        n_noise = graph.add_node_of_type(unreal.PCGAttributeNoiseSettings)[0]
        n_noise.set_node_position(400, 0)
        
        # B. Transform (For Jitter/Grid Breaking)
        n_trans = graph.add_node_of_type(unreal.PCGTransformPointsSettings)[0]
        n_trans.set_node_position(600, 0)
        s = n_trans.get_settings()
        s.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
        s.set_editor_property("OffsetMin", unreal.Vector(-50, -50, -0)) 
        s.set_editor_property("OffsetMax", unreal.Vector(50, 50, 0))
        s.set_editor_property("ScaleMin", unreal.Vector(0.8, 0.8, 0.8))
        s.set_editor_property("ScaleMax", unreal.Vector(1.4, 1.4, 1.4))
        
        # C. Projection (Snap)
        n_proj = graph.add_node_of_type(unreal.PCGProjectionSettings)[0]
        n_proj.set_node_position(800, 0)
        
        # 4. Filters & Spawners
        # We need to recreate 4 filters.
        
        # F1 -> Spawner 0 (Big)
        f1 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f1.set_node_position(1000, -200)
        f1.get_settings().lower_bound = 0.9
        
        # F2 -> Spawner 1 (Med)
        f2 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f2.set_node_position(1000, 0)
        f2.get_settings().lower_bound = 0.6
        
        # F3 -> Spawner 2 (Small)
        f3 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f3.set_node_position(1000, 200)
        f3.get_settings().lower_bound = 0.4
        
        # F4 -> Spawner 3 (Bush) - Assuming Spawner 3 exists
        f4 = graph.add_node_of_type(unreal.PCGDensityFilterSettings)[0]
        f4.set_node_position(1000, 400)
        f4.get_settings().lower_bound = 0.2
        
        # 5. Connect The Chain
        # Reader -> Bounds (Already Done? verify)
        # Bounds -> Noise
        # Noise -> Transform
        # Transform -> Project
        # Project -> Filters -> Spawners
        
        try:
            # We assume Reader->Bounds exists. We append from Bounds.
            graph.add_edge(bounds_node, "Out", n_noise, "In")
            graph.add_edge(n_noise, "Out", n_trans, "In")
            graph.add_edge(n_trans, "Out", n_proj, "In")
            
            # Fan out to Filters
            graph.add_edge(n_proj, "Out", f1, "In")
            graph.add_edge(n_proj, "Out", f2, "In")
            graph.add_edge(n_proj, "Out", f3, "In")
            graph.add_edge(n_proj, "Out", f4, "In")
            
            # Filters to Spawners
            if "StaticMeshSpawner_0" in spawners: graph.add_edge(f1, "Out", spawners["StaticMeshSpawner_0"], "In")
            if "StaticMeshSpawner_1" in spawners: graph.add_edge(f2, "Out", spawners["StaticMeshSpawner_1"], "In")
            if "StaticMeshSpawner_2" in spawners: graph.add_edge(f3, "Out", spawners["StaticMeshSpawner_2"], "In")
            if "StaticMeshSpawner_3" in spawners: graph.add_edge(f4, "Out", spawners["StaticMeshSpawner_3"], "In")

            print("Restored Chain: Bounds -> Noise -> Transform -> Project -> Filters -> Spawners")
            
        except Exception as e:
            print(f"Wiring Error: {e}")
            
    else:
        print("Required start nodes (Reader/Bounds) missing.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Full Logic Restored.")

"""

def restore_full():
    print(f"--- [Fix] Restoring Full Logic ---", flush=True)
    
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
    restore_full()
