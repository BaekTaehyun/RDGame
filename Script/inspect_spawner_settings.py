import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Inspect] Spawner Settings ---")

graph = unreal.load_asset(graph_path)
if graph:
    for n in graph.nodes:
        if "StaticMeshSpawner" in n.get_name():
            print(f"\\nNode: {n.get_name()}")
            s = n.get_settings()
            
            # Inspect Mesh Entries
            # Property: 'MeshEntries' (Array of FPCGStaticMeshSpawnerEntry)
            try:
                entries = s.get_editor_property("MeshEntries")
                print(f"  Mesh Entries Count: {len(entries)}")
                
                for idx, e in enumerate(entries):
                    # FPCGStaticMeshSpawnerEntry has 'Mesh' (SoftObjectPath or similar)
                    try:
                        mesh = e.get_editor_property("Mesh")
                        # Mesh might be a SoftObjectPath, get string
                        print(f"    [{idx}] Mesh: {mesh}")
                        
                        # Also check Weight
                        weight = e.get_editor_property("Weight")
                        print(f"        Weight: {weight}")
                    except: 
                        print(f"    [{idx}] (Cannot read Mesh property)")
            except Exception as e:
                print(f"  Error reading MeshEntries: {e}")
"""

def inspect_spawner():
    print(f"--- [Inspect] Spawner ---", flush=True)
    
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
    inspect_spawner()
