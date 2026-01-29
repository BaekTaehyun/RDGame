import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Connect] Final V2 ---")

graph = unreal.load_asset(graph_path)
if graph:
    # Build Map
    node_map = {}
    for n in graph.nodes:
        node_map[n.get_name()] = n

    def find_node(partial):
        for name, n in node_map.items():
            if partial in name: return n
        return None

    land = find_node("GetLandscapeData")
    sampler = find_node("SurfaceSampler")
    
    # Readers (find both)
    readers = [n for n in graph.nodes if "DungeonDataReader" in n.get_name()]
    wall = readers[0] if len(readers) > 0 else None
    floor = readers[1] if len(readers) > 1 else None
    
    union = find_node("Union")
    bounds = find_node("BoundsModifier")
    diff = find_node("Difference")
    trans = find_node("TransformPoints")
    
    filters = [n for n in graph.nodes if "DensityFilter" in n.get_name()]

    pairs = [
        (land, "Out", sampler, "Surface"),
        (wall, "Out", union, "In"),
        (floor, "Out", union, "In"),
        (union, "Out", bounds, "In"),
        (sampler, "Out", diff, "Source"),
        (bounds, "Out", diff, "Differences"),
        (diff, "Out", trans, "In")
    ]
    
    success_count = 0
    
    for src, sp, dst, dp in pairs:
        if src and dst:
            print(f"Connecting {src.get_name()}:{sp} -> {dst.get_name()}:{dp}")
            
            # Method 1
            try:
                res = graph.add_edge(src, unreal.Name(sp), dst, unreal.Name(dp))
                if res: 
                    print("  [Graph.AddEdge] Success")
                    success_count += 1
                else:
                    # Method 2 Fallback
                    try:
                        res2 = src.add_edge_to(unreal.Name(sp), dst, unreal.Name(dp))
                        if res2: 
                            print("  [Node.AddEdgeTo] Success")
                            success_count += 1
                        else:
                            print("  [Both Failed]")
                    except: print("  [Both Failed]")
            except Exception as e:
                print(f"  [Error] {e}")
                
    # Filters
    if trans:
        for f in filters:
            try: graph.add_edge(trans, unreal.Name("Out"), f, unreal.Name("In"))
            except: pass

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
    
    print(f"Total Successes: {success_count}/{len(pairs)}")
"""

def force_connect_final_v2():
    print(f"--- [Connect] Final V2 ---", flush=True)
    
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
    force_connect_final_v2()
