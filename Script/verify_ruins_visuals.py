import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def verify_visuals():
    print(f"--- [Verification] Checking Ruins Visibility (Debug Mode) ---", flush=True)
    
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
        
        code_check = f"""
import unreal
import traceback

try:
    print("Beginning Check...")
    graph = unreal.load_asset("{GRAPH_PATH}")
    found_pillar = "None"
    filter_range = "Unknown"

    if graph:
        for n in graph.nodes:
            name = n.get_name()
            
            # Check Spawner
            if name == "StaticMeshSpawner_4":
                print("Found Spawner_4")
                s = n.get_settings()
                try:
                    # Try accessing Mesh Selector
                    # In Python API, accessing struct arrays is hard.
                    # But we can try to print the object
                    print(f"Settings: {{s}}")
                    
                    # Assume Weighted Entry
                    # PCGMeshSelectorWeighted
                    sel = s.get_editor_property("mesh_selector_parameters")
                    entries = sel.get_editor_property("MeshEntries")
                    print(f"Entries: {{len(entries)}}")
                    
                    found_pillar = "False"
                    for e in entries:
                        desc = e.get_editor_property("Descriptor")
                        mesh = desc.get_editor_property("StaticMesh")
                        if mesh:
                             print(f" Mesh: {{mesh.get_name()}}")
                             if "Pillar" in mesh.get_name():
                                 found_pillar = "True"
                except Exception as e_inner:
                    print(f"Spawner Error: {{e_inner}}")
                    traceback.print_exc()

            if name == "DensityFilter_1":
                s = n.get_settings()
                lb = s.get_editor_property("LowerBound")
                ub = s.get_editor_property("UpperBound")
                filter_range = f"{{lb:.2f}}~{{ub:.2f}}"
                
    print(f"RESULT | Pillar: {{found_pillar}} | Range: {{filter_range}}")

except Exception as e:
    print(f"Fatal Check Error: {{e}}")
    traceback.print_exc()
"""
        res = rpc("tools/call", {
            "name": "execute_unreal_script",
            "arguments": {"code": code_check}
        }, True)
        
        print(f"   Output: {res['result'].get('output', 'No Output')}", flush=True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    verify_visuals()
