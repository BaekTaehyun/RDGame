import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Connect] Final V3 ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes_list = list(graph.nodes)
    
    land =     next((n for n in nodes_list if "GetLandscapeData" in n.get_name()), None)
    sampler =  next((n for n in nodes_list if "SurfaceSampler" in n.get_name()), None)
    union =    next((n for n in nodes_list if "Union" in n.get_name()), None)
    bounds =   next((n for n in nodes_list if "BoundsModifier" in n.get_name()), None)
    diff =     next((n for n in nodes_list if "Difference" in n.get_name()), None)
    trans =    next((n for n in nodes_list if "TransformPoints" in n.get_name()), None)
    
    readers = [n for n in nodes_list if "DungeonDataReader" in n.get_name()]
    wall = readers[0] if len(readers) > 0 else None
    floor = readers[1] if len(readers) > 1 else None
    
    filters = [n for n in nodes_list if "DensityFilter" in n.get_name()]
    
    # Pairs (Verified Pin Names)
    # Landscape:Out -> Sampler:Surface
    # Readers:Out -> Union:In
    # Union:Out -> Bounds:In
    # Sampler:Out -> Diff:Source
    # Bounds:Out -> Diff:Differences
    # Diff:Out -> Trans:In
    # Trans:Out -> Filters:In

    pairs = []
    if land and sampler: pairs.append((land, "Out", sampler, "Surface"))
    if wall and union: pairs.append((wall, "Out", union, "In"))
    if floor and union: pairs.append((floor, "Out", union, "In"))
    if union and bounds: pairs.append((union, "Out", bounds, "In"))
    if sampler and diff: pairs.append((sampler, "Out", diff, "Source"))
    if bounds and diff: pairs.append((bounds, "Out", diff, "Differences"))
    if diff and trans: pairs.append((diff, "Out", trans, "In"))
    if trans:
        for f in filters:
            pairs.append((trans, "Out", f, "In"))
            
    success_count = 0
    
    for src, sp, dst, dp in pairs:
        print(f"Wiring {src.get_name()}:{sp} -> {dst.get_name()}:{dp}")
        try:
             # Try add_edge directly
             res = graph.add_edge(src, unreal.Name(sp), dst, unreal.Name(dp))
             if res:
                 print("  [Success]")
                 success_count += 1
             else:
                 print("  [Failed (None)]")
                 # Check if connection already exists?
        except Exception as e:
             print(f"  [Error] {e}")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
    
    print(f"Total Connected: {success_count}/{len(pairs)}")
"""

def force_connect_v3():
    print(f"--- [Connect] Final V3 ---", flush=True)
    
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
    force_connect_v3()
