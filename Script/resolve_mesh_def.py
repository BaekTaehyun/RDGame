import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

# Inner Python code to run in Unreal
PYTHON_CODE = """
import unreal

spawner_node_name = "StaticMeshSpawner_4"
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
                print(f"--- Inspecting Settings for {spawner_node_name} ---")
                # Dump attrs
                attrs = dir(settings)
                mesh_attr = None
                for a in attrs:
                    if "mesh" in a.lower() and "entry" in a.lower(): # e.g. mesh_entries
                        print(f"Candidate: {a}")
                        mesh_attr = a
                    if a.lower() == "meshes":
                        print(f"Candidate: {a}")
                        mesh_attr = a

                if mesh_attr:
                    print(f"-> Selected Property: {mesh_attr}")
                    
                    # Create Entry objects?
                    # Since it is likely an array of Structs (PCGStaticMeshSpawnerEntry), we can't just pass strings.
                    # We might need to construct them.
                    # Or simpler: The settings object usually makes this hard in Python.
                    # BUT let's try to set it.
                    
                    try:
                        # Construct Entries
                        entries = []
                        for p in paths:
                            # Try to create an entry
                            # Class: unreal.PCGStaticMeshSpawnerEntry
                            # Note: In some versions it's a struct inside the settings.
                            
                            mesh_obj = unreal.load_asset(p)
                            if not mesh_obj:
                                print(f"Warning: Asset not found {p}")
                                continue
                                
                            entry = unreal.PCGStaticMeshSpawnerEntry()
                            entry.mesh = mesh_obj
                            entry.weight = 1
                            entries.append(entry)
                        
                        setattr(settings, mesh_attr, entries)
                        print(f"SUCCESS: Assigned {len(entries)} meshes to {mesh_attr}")
                        
                        # Save
                        unreal.EditorAssetLibrary.save_loaded_asset(graph)
                        print("Asset Saved.")
                        found = True
                    except Exception as e:
                        print(f"Error setting meshes: {e}")
                else:
                    print("Could not find a 'Mesh' property on Settings.")
            break

if not found:
    print("Could not find node or apply settings.")
"""

def resolve_and_fix():
    print(f"--- [Fix] Resolving Spawner Meshes ---", flush=True)
    
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
    resolve_and_fix()
