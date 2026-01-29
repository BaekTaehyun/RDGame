import sys
import json
import subprocess
import time
import os

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
OUTPUT_FILE = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Temp_PCG_Audit.txt"

PYTHON_CODE = f"""
import unreal
import os

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
out_path = r"{OUTPUT_FILE}"

with open(out_path, "w", encoding='utf-8') as f:
    f.write(f"--- PCG Audit Report: {{graph_path}} ---\\n")
    
    try:
        graph = unreal.load_asset(graph_path)
        if not graph:
            f.write("Error: Graph asset not found\\n")
        else:
            nodes = sorted(graph.nodes, key=lambda n: n.get_name())
            f.write(f"Total Nodes: {{len(nodes)}}\\n")
            
            for n in nodes:
                name = n.get_name()
                node_cls = n.get_class().get_name()
                
                # Check for Spawner
                if "StaticMeshSpawner" in node_cls or "Spawner" in name:
                    f.write(f"\\n[Node: {{name}}] ({{node_cls}})\\n")
                    s = n.get_settings()
                    
                    try:
                        entries = []
                        # Try direct property first
                        try:
                            entries = s.get_editor_property("MeshEntries")
                        except:
                            pass
                            
                        # Try selector second
                        if not entries:
                            try:
                                sel = s.get_editor_property("mesh_selector_parameters")
                                entries = sel.get_editor_property("MeshEntries")
                            except:
                                pass
                        
                        if not entries:
                            f.write("   (No MeshEntries Found)\\n")
                        else:
                            for idx, e in enumerate(entries):
                                try:
                                    desc = e.get_editor_property("Descriptor")
                                    mesh = desc.get_editor_property("StaticMesh")
                                    w = e.get_editor_property("Weight")
                                    m_name = mesh.get_name() if mesh else "None"
                                    f.write(f"   Entry {{idx}}: {{m_name}} (Weight={{w}})\\n")
                                except:
                                    f.write(f"   Entry {{idx}}: <Error Reading>\\n")
                                    
                    except Exception as ex:
                        f.write(f"   Error inspecting settings: {{ex}}\\n")

    except Exception as e_fatal:
        f.write(f"Fatal Audit Error: {{e_fatal}}\\n")

print(f"Audit written to: {{out_path}}")
"""

def audit_to_file():
    print(f"--- [Audit] Writing Spawner Data to File ---", flush=True)
    
    # Remove old file if exists
    if os.path.exists(OUTPUT_FILE):
        try: os.remove(OUTPUT_FILE)
        except: pass

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

        res = rpc("tools/call", {
            "name": "execute_unreal_script",
            "arguments": {"code": PYTHON_CODE}
        }, True)
        
        # Wait a bit for file write
        time.sleep(1.0)
        
        if os.path.exists(OUTPUT_FILE):
            print("\n--- FILE CONTENT ---")
            with open(OUTPUT_FILE, "r", encoding='utf-8') as f:
                print(f.read())
        else:
            print("[Error] Output file was NOT created.")

    except Exception as e:
        print(f"[Error] {e}", flush=True)
    finally:
        proc.terminate()

if __name__ == "__main__":
    audit_to_file()
