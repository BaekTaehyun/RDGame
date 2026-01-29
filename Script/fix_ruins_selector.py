import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

spawner_node_name = "StaticMeshSpawner_4" # Ruins Spawner
paths = [
    "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged",
    "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar.SM_Stone_Pillar",
    "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Column_Destroyed.SM_Stone_Column_Destroyed"
]

graph = unreal.load_asset("/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood")
found = False

if graph:
    for n in graph.nodes:
        if n.get_name() == spawner_node_name:
            settings = n.get_settings()
            if settings:
                print(f"--- Fixing Selector for {spawner_node_name} ---")
                
                # Check Selector
                selector = settings.mesh_selector_parameters
                if not selector:
                    print("Error: No Mesh Selector found.")
                    continue
                
                print(f"Selector Class: {selector.get_class().get_name()}") # Expect PCGMeshSelectorWeighted
                
                # Construct Entries
                # Properties of PCGMeshSelectorWeightedEntry usually: Mesh, Weight.
                # Property of Selector: 'MeshEntries' (Array)
                
                # We need to find the correct Struct for the Entry.
                # It is likely 'PCGMeshSelectorWeightedEntry'.
                
                try:
                    entries = []
                    for p in paths:
                        mesh_obj = unreal.load_asset(p)
                        if not mesh_obj:
                            print(f"Warning: Asset not found {p}")
                            continue
                        
                        # Create Entry
                        entry = unreal.PCGMeshSelectorWeightedEntry()
                        
                        # Configure Descriptor
                        # property 'Descriptor' is a Struct, so we must access it, modify, set back?
                        # Or modify in place if it's a reference (unlikely for structs in Python)
                        # Usually: get -> modify -> set.
                        
                        desc = entry.get_editor_property("Descriptor")
                        
                        # Set StaticMesh. 
                        # Note: PCGSoftISMComponentDescriptor might expect a SoftObjectPath or the StaticMesh object.
                        # Trying StaticMesh object first.
                        try:
                            desc.set_editor_property("StaticMesh", mesh_obj)
                        except Exception as e:
                            print(f"Failed to set StaticMesh on descriptor: {e}")
                            # Fallback: Maybe it expects SoftObjectPath?
                            # desc.set_editor_property("StaticMesh", list(p)) ??
                            continue

                        # Set back to entry
                        entry.set_editor_property("Descriptor", desc)
                        
                        # Set Weight
                        entry.set_editor_property("Weight", 1)
                        
                        entries.append(entry)
                    
                    if not entries:
                        print("Error: No valid entries created.")
                    else:
                        # Apply to Selector
                        selector.set_editor_property("MeshEntries", entries)
                        print(f"SUCCESS: Assigned {len(entries)} meshes to Selector.")
                        
                        # Force Save
                        unreal.EditorAssetLibrary.save_loaded_asset(graph)
                        print("Asset Saved.")
                        found = True
                    
                except Exception as e:
                    print(f"Error applying fix: {e}")
            break

if not found:
    print("Node not found")
"""

def fix_selector():
    print(f"--- [Fix] Applying Meshes to Selector ---", flush=True)
    
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
    fix_selector()
