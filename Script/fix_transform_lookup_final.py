import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Connecting Wall Data (Final Attempt) ---")

graph = unreal.load_asset(graph_path)
if graph:
    wall_node = None
    noise_node = None
    trans_node = None
    proj_node = None
    
    # 1. Identify Nodes
    for n in graph.nodes:
        name = n.get_name()
        
        if name == "DungeonDataReader_2": wall_node = n
        if "AttributeNoise" in name: noise_node = n
        if "Projection" in name: proj_node = n
        
        # Robust Transform Finder: By Class, excluding Ruins
        if n.get_settings() and "PCGTransformPointsSettings" in n.get_settings().get_class().get_name():
            if name != "TransformPoints_2":
                trans_node = n

    # 2. Re-wire Chain
    if wall_node and noise_node and trans_node and proj_node:
        try:
            # Wall -> Noise
            graph.add_edge(wall_node, "Out", noise_node, "In")
            
            # Noise -> Transform
            graph.add_edge(noise_node, "Out", trans_node, "In")
            
            # Transform -> Projection
            graph.add_edge(trans_node, "Out", proj_node, "In")
            
            # Veriy Proj -> Filters
            targets = ["DensityFilter_1", "DensityFilter_2", "DensityFilter_3", "DensityFilter_4"]
            for t in targets:
                # Find T
                target_node = None
                for n2 in graph.nodes:
                    if n2.get_name() == t: target_node = n2
                if target_node:
                    graph.add_edge(proj_node, "Out", target_node, "In")
            
            print(f"Chain Connected: Wall -> Noise -> Transform({trans_node.get_name()}) -> Project -> Filters")
            
        except Exception as e:
            print(f"Connection Failed: {e}")
    else:
        print("Missing Nodes:")
        if not wall_node: print("  Reader_2 Missing")
        if not noise_node: print("  Noise Missing")
        if not trans_node: print("  Transform Missing (No Valid Transform Found)")
        if not proj_node: print("  Project Missing")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Wall Logic Applied.")

"""

def fix_lookup():
    print(f"--- [Fix] Final Lookup ---", flush=True)
    
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
    fix_lookup()
