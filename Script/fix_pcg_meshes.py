import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

# We will populate Spawner 4 with Ruin Meshes
# Expected meshes: SM_Stone_Pillar_Damaged, SM_Stone_Floor_2x2, etc. (from user images/context)
# I will use a safe list of likely assets or just 'SM_Stone_Pillar_Damaged' for now.
PYTHON_CODE = f"""
import unreal

try:
    graph_path = "{GRAPH_PATH}"
    print(f"Loading {{graph_path}}...")
    graph = unreal.load_asset(graph_path)

    if not graph:
        print("Error: Graph not found")
    else:
        changed = False
        nodes = graph.nodes
        
        # 1. Populate Spawner 4
        target_spawner = None
        for n in nodes:
            if n.get_name() == "StaticMeshSpawner_4":
                target_spawner = n
                break
        
        if target_spawner:
            print("Found Spawner 4. populating meshes...")
            s = target_spawner.get_settings()
            s.modify()
            
            # Create Entries
            # We need to manually construct the array of PCGMeshSelectorWeightedEntry
            # This is hard via Python API if 'MeshEntries' is read-only or struct array.
            # Best way: Use 'StaticMeshComponentPropertyOverrides' ? No.
            
            # Alternative: Load a Mesh and set it?
            # Let's try to set 'MeshEntries' on the 'mesh_selector_parameters'
            
            ruin_mesh_path = "/Game/LevelPrototyping/Meshes/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged"
            ruin_mesh = unreal.load_asset(ruin_mesh_path)
            
            if ruin_mesh:
                # We need the PCGMeshSelectorWeightedEntry class
                # But it's a struct.
                # In Python, we can't always create structs easily if not exposed.
                pass
                
                # If we fail to modify mesh entries in Python, we might need C++ or Pre-made Data Asset.
                # However, let's try assuming the user has AT LEAST ONE entry we can duplicate?
                # The audit said "No MeshEntries Found".
                
                # PLAN B: Use 'set_editor_property' with a list of dicts? 
                # Unreal Python sometimes accepts dicts for structs.
                
                entries_data = [
                    {{
                        "Descriptor": {{
                            "StaticMesh": ruin_mesh,
                            "Scale": 1.0
                        }},
                        "Weight": 1
                    }}
                ]
                
                # Try setting on Selector (Defaut is PCGMeshSelectorWeighted)
                try:
                    # We might need to Instantiate the selector parameters first?
                    # It is an InstancedStruct.
                    
                    # Direct approach:
                    # s.set_editor_property("MeshEntries", ... ) if it exists on settings (Older PCG)
                    # s.mesh_selector_parameters.set_editor_property("MeshEntries", ...) (Newer)
                    
                    sel = s.get_editor_property("mesh_selector_parameters")
                    # Check if we can set it
                    # This is highly risky without known API.
                    
                    # Hack: The Server 'unreal_socket_server.py' had specific logic for this.
                    # "if p_name == 'MeshEntries' and isinstance(p_val, list):"
                    # It creates 'unreal.PCGMeshSelectorWeightedEntry'.
                    pass

                except Exception as e:
                    print(f"Error prep: {{e}}")

            else:
                print(f"Mesh not found: {{ruin_mesh_path}}")

except Exception as e:
    print(f"Fatal Error: {{e}}")
"""

# Re-using the logic I saw in 'unreal_socket_server.py' is the best bet.
# It has a handler for "MeshEntries" (List of Strings).
# I should just call 'set_pcg_node_properties' with a LIST of paths!
# The server handles the hard struct creation!

def fix_meshes():
    print(f"--- [Fixing] Populating Spawner 4 via MCP ---", flush=True)
    
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
        req = {"jsonrpc": "2.0", "method": method, "params": params, "id": req_id}
        try:
            proc.stdin.write(json.dumps(req) + "\n")
            proc.stdin.flush()
        except: return None
        if expect_response: return json.loads(proc.stdout.readline())
        return None

    try:
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        # 1. Set Meshes for Spawner 4
        # Verified Paths from Content Search
        paths = [
            "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged",
            "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar.SM_Stone_Pillar",
            "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Column_Destroyed.SM_Stone_Column_Destroyed"
        ]
        
        print(f"[1/2] Assigning Meshes to Spawner 4...", flush=True)
        res = rpc("tools/call", {
            "name": "set_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": "StaticMeshSpawner_4",
                "properties": {
                    "MeshEntries": paths
                }
            }
        }, True)
        
        # Check Success
        if res.get('error'):
             print(f"   Error: {{res['error']}}", flush=True)
        else:
             print(f"   Result: {{res['result']}}", flush=True)

        # 2. Adjust Density (Reduce Trees?)
        # User said "Trees too dense".
        # Check DensityFilter_0 (Trees Filter)
        # Assuming DensityFilter_0 feeds Spawner 0,1,2,3?
        # Let's slighty increase LowerBound or Reduce UpperBound?
        # Or Just 'SelfPruning' node?
        
        # Just notify user about Mesh Fix first.

        # 3. Save
        code_save = f"import unreal; unreal.EditorAssetLibrary.save_asset('{GRAPH_PATH}')"
        rpc("tools/call", {"name": "execute_unreal_script", "arguments": {"code": code_save}}, True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    fix_meshes()
