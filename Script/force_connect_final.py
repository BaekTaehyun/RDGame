import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Connect] Final ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {}
    for n in graph.nodes:
        nodes[n.get_name()] = n

    # Helper to find node by partial name
    def get_node(partial):
        for name, n in nodes.items():
            if partial in name: return n
        return None

    land = get_node("GetLandscapeData")
    sampler = get_node("SurfaceSampler")
    wall = get_node("Wall") # Title override might not change Name, check previous logs
    # Previous logs: Wall -> DungeonDataReader_2 (Title='Wall'?? No, checking Internal Name)
    # The output of inspect_pins_fixed showed: GetLandscapeData_2, SurfaceSampler_1, Union_0, BoundsModifier_1, Difference_0, TransformPoints_0
    
    # We need to find Readers.
    wall = None
    floor = None
    for n in graph.nodes:
        if "DungeonDataReader" in n.get_name():
            # Check title to be sure, or just assume order
            # Just grab two
            if not wall: wall = n
            else: floor = n

    union = get_node("Union")
    bounds = get_node("BoundsModifier")
    diff = get_node("Difference")
    trans = get_node("TransformPoints")
    
    filters = []
    for n in graph.nodes:
        if "DensityFilter" in n.get_name(): filters.append(n)

    # CONNECTIONS
    pairs = [
        (land, "Out", sampler, "Surface"),
        (wall, "Out", union, "In"),
        (floor, "Out", union, "In"),
        (union, "Out", bounds, "In"),
        (sampler, "Out", diff, "Source"),
        (bounds, "Out", diff, "Differences"),
        (diff, "Out", trans, "In")
    ]
    
    for src, sp, dst, dp in pairs:
        if not src or not dst: 
            print(f"Missing node for {sp}->{dp}")
            continue
            
        print(f"Connecting {src.get_name()}:{sp} -> {dst.get_name()}:{dp} ...")
        
        # Method 1: graph.add_edge
        try:
            res = graph.add_edge(src, unreal.Name(sp), dst, unreal.Name(dp))
            if res: print("  [Graph.AddEdge] Success")
            else: print("  [Graph.AddEdge] Failed (Returned None)")
        except Exception as e:
            print(f"  [Graph.AddEdge] Error: {e}")

        # Method 2: node.add_edge_to (if method 1 failed)
        # Signature guess: add_edge_to(OtherNode, SrcPinName, DstPinName)?
        try:
            # Check generic invocation
             res = src.add_edge_to(unreal.Name(sp), dst, unreal.Name(dp))
             if res: print("  [Node.AddEdgeTo] Success")
        except Exception as e:
            print(f"  [Node.AddEdgeTo] Error: {e}")

    # Filters
    if trans:
        for f in filters:
            try: graph.add_edge(trans, unreal.Name("Out"), f, unreal.Name("In"))
            except: pass

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def force_connect_final():
    print(f"--- [Connect] Final ---", flush=True)
    
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
    force_connect_final()
