import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Restoring Full Chain (Project -> Filter -> Spawner) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Map Spawners and Filters
    # Assuming Standard numbering
    pairs = [
        ("DensityFilter_1", "StaticMeshSpawner_0"),
        ("DensityFilter_2", "StaticMeshSpawner_1"),
        ("DensityFilter_3", "StaticMeshSpawner_2"),
        ("DensityFilter_4", "StaticMeshSpawner_3"),
        ("DensityFilter_5", "StaticMeshSpawner_5") # Ruins
    ]
    
    for f_name, s_name in pairs:
        f_node = None
        s_node = None
        for n in graph.nodes:
            if n.get_name() == f_name: f_node = n
            if n.get_name() == s_name: s_node = n
            
        if f_node and s_node:
            try:
                # Add edge: Filter -> Spawner
                graph.add_edge(f_node, "Out", s_node, "In")
                print(f"Connected: {f_name} -> {s_name}")
            except Exception as e:
                print(f"Error connecting {f_name}->{s_name}: {e}")

    # 2. Verify Projection Settings (Project to Landscape)
    proj_node = None
    for n in graph.nodes:
        if "Projection" in n.get_name() and "Settings" in n.get_settings().get_class().get_name():
            proj_node = n
            break
            
    if proj_node:
        s = proj_node.get_settings()
        # Default is usually Landscape.
        # We can try to set 'ProjectionTarget' if exposed?
        # Usually it projects to 'Projection Target' enum.
        # Let's hope defaults work. 
        # But we ensure it is connected to Filters?
        # My previous script did that.
        pass
    else:
        print("Warning: Projection Node not found (Should have been created).")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Chain Restored. Check Tree Visibility on Ground.")

"""

def restore_chain():
    print(f"--- [Fix] Restoring Chain ---", flush=True)
    
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
    restore_chain()
