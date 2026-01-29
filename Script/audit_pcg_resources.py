import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

# Safely print structure without 'f-string inside f-string' complexity
PYTHON_CODE = """
import unreal

try:
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"Loading {graph_path}...")
    graph = unreal.load_asset(graph_path)

    if not graph:
        print("Error: Graph not found")
    else:
        print("-" * 50)
        nodes = sorted(graph.nodes, key=lambda n: n.get_name())
        
        for n in nodes:
            name = n.get_name()
            if "StaticMeshSpawner" in n.get_class().get_name() or "Spawner" in name:
                print(f"[Node: {name}]")
                s = n.get_settings()
                
                try:
                    entries = []
                    try:
                        # Try direct
                        entries = s.get_editor_property("MeshEntries")
                    except:
                        # Try selector
                        sel = s.get_editor_property("mesh_selector_parameters")
                        entries = sel.get_editor_property("MeshEntries")

                    if not entries:
                        print("   (No Entries Found)")
                    else:
                        for idx, e in enumerate(entries):
                            try:
                                desc = e.get_editor_property("Descriptor")
                                mesh = desc.get_editor_property("StaticMesh")
                                w = e.get_editor_property("Weight")
                                m_name = mesh.get_name() if mesh else "None"
                                print(f"   [{idx}] {m_name} (W={w})")
                            except Exception as ex_entry:
                                print(f"   [Error reading Entry {idx}]: {ex_entry}")
                
                except Exception as e_inner:
                    print(f"   Error inspecting settings: {e_inner}")
                print("-" * 20)

except Exception as e:
    print(f"Fatal Error: {e}")
"""

def audit_pcg():
    print(f"--- [Audit] Inspecting All Spawners & Meshes ---", flush=True)
    
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

        print("[1/1] Running Audit...", flush=True)
        res = rpc("tools/call", {
            "name": "execute_unreal_script",
            "arguments": {"code": PYTHON_CODE}
        }, True)
        
        out = res['result'].get('output', 'No Output')
        print(out, flush=True)

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    audit_pcg()
