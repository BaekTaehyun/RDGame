import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Inspect Static Mesh Spawner Meshes ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        
        class_name = settings.get_class().get_name()
        
        if "StaticMeshSpawner" in class_name:
            print(f"\\n[SPAWNER] {class_name}")
            print(f"Node Position: ({node.position_x}, {node.position_y})")
            
            # List ALL properties
            print("\\nAll Properties:")
            for prop in dir(settings):
                if not prop.startswith('_') and not prop.startswith('get') and not prop.startswith('set'):
                    try:
                        val = getattr(settings, prop)
                        if not callable(val):
                            print(f"  {prop}: {val}")
                    except: pass
            
            # Try various mesh-related properties
            print("\\nMesh Properties:")
            try:
                sel_type = settings.get_editor_property("mesh_selector_type")
                print(f"  mesh_selector_type: {sel_type}")
            except: print("  mesh_selector_type: N/A")
            
            try:
                sel_instance = settings.get_editor_property("mesh_selector_instance")
                print(f"  mesh_selector_instance: {sel_instance}")
                if sel_instance:
                    print(f"    Type: {sel_instance.get_class().get_name()}")
                    # Check for meshes in various properties
                    for p in dir(sel_instance):
                        if 'mesh' in p.lower() and not p.startswith('_'):
                            try:
                                v = getattr(sel_instance, p)
                                if not callable(v):
                                    print(f"    {p}: {v}")
                            except: pass
            except Exception as e:
                print(f"  mesh_selector_instance error: {e}")
            
            try:
                params = settings.get_editor_property("mesh_selector_parameters")
                print(f"  mesh_selector_parameters: {params}")
                if params:
                    print(f"    Type: {type(params)}")
                    # Check mesh entries in params
                    try:
                        entries = params.get_editor_property("mesh_entries")
                        print(f"    mesh_entries: {entries}")
                    except: pass
            except Exception as e:
                print(f"  mesh_selector_parameters error: {e}")

print("\\n=== Done ===")
"""

def inspect_meshes():
    print(f"--- [Inspect Spawner Meshes] ---", flush=True)
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
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
    inspect_meshes()
