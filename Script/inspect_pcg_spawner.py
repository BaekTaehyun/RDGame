import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood" 
target_graph = unreal.load_asset(graph_path)

if target_graph:
    print(f"Inspecting Graph: {target_graph.get_name()}")
    
    for node in target_graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        
        class_name = settings.get_class().get_name()
        
        if "PCGStaticMeshSpawnerSettings" in class_name:
            print(f"--- Spawner Node Class: {class_name} ---")
            for p in dir(settings):
                if "mesh" in p.lower():
                    print(f"Property Candidate: {p}")
            
            # Try 'static_mesh_entries'
            try:
                entries = settings.get_editor_property("static_mesh_entries")
                if entries:
                    print(f"Static Mesh Entries Found: {len(entries)}")
            except: pass
            else:
                print(f"Mesh Entries Count: {len(entries)}")
                for i, entry in enumerate(entries):
                    # Entry is usually Struct 'PCGStaticMeshSpawnerEntry'
                    # It has 'descriptor' struct -> 'static_mesh'
                    
                    # Try to access descriptor
                    try:
                        desc = entry.get_editor_property("descriptor")
                        mesh = desc.get_editor_property("static_mesh")
                        print(f"  [{i}] Mesh: {mesh}")
                        if mesh: print(f"       Name: {mesh.get_name()}")
                    except Exception as e:
                        print(f"  [{i}] Error accessing mesh: {e}")
                        # fallback print
                        print(f"  [{i}] Raw: {entry}")

"""

def inspect_spawner():
    print(f"--- [Inspect] PCG Spawner ---", flush=True)
    
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
