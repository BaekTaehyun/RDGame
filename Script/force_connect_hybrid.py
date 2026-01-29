import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Hybrid] Force Connect (Shotgun) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Map Nodes
    nodes = {
        "Wall": None, "Floor": None, "Land": None, "Sampler": None,
        "Union": None, "Bounds": None, "Diff": None, "Trans": None,
        "Filters": []
    }
    
    for n in graph.nodes:
        nm = n.get_name()
        
        # Readers
        if "DungeonDataReader" in nm:
            t = ""
            try: t = n.get_editor_property("NodeTitleOverride")
            except: pass
            if "Wall" in t: nodes["Wall"] = n
            elif "Floor" in t: nodes["Floor"] = n
            
        # Others
        if "GetLandscape" in nm: nodes["Land"] = n
        if "SurfaceSampler" in nm: nodes["Sampler"] = n
        if "Union" in nm: nodes["Union"] = n
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "Difference" in nm: nodes["Diff"] = n
        if "TransformPoints" in nm: nodes["Trans"] = n
        
        # Filter (Collect all)
        if "DensityFilter" in nm: nodes["Filters"].append(n)

    # 2. Connection Helpers
    def try_connect(src, dst, src_opts, dst_opts):
        if not src or not dst: return
        
        # If strings, make list
        if isinstance(src_opts, str): src_opts = [src_opts]
        if isinstance(dst_opts, str): dst_opts = [dst_opts]
        
        success = False
        for sp in src_opts:
            for dp in dst_opts:
                if success: break
                try:
                    # explicit unreal.Name
                    res = graph.add_edge(src, unreal.Name(sp), dst, unreal.Name(dp))
                    if res:
                        print(f"Connected {src.get_name()}({sp}) -> {dst.get_name()}({dp})")
                        success = True
                except: pass
                
        if not success:
            print(f"Failed to connect {src.get_name()} -> {dst.get_name()}")

    # 3. Chains
    
    # Layer 1: Land -> Sampler
    # Land output: 'Data', 'Out'
    # Sampler input: 'Surface', 'In'
    try_connect(nodes["Land"], nodes["Sampler"], ["Data", "Out"], ["Surface", "In"])
    
    # Layer 2: Readers -> Union
    try_connect(nodes["Wall"], nodes["Union"], "Out", "In")
    try_connect(nodes["Floor"], nodes["Union"], "Out", "In")
    
    # Union -> Bounds
    try_connect(nodes["Union"], nodes["Bounds"], "Out", "In")
    
    # Exclusion: Sampler -> Diff (Source)
    # Diff inputs: 'Source', 'Difference' (or 'Differences', 'Target')
    try_connect(nodes["Sampler"], nodes["Diff"], "Out", ["Source"])
    
    # Exclusion: Bounds -> Diff (Subtraction)
    try_connect(nodes["Bounds"], nodes["Diff"], "Out", ["Differences", "Difference", "Target", "Subtract"])
    
    # Result -> Transform
    try_connect(nodes["Diff"], nodes["Trans"], "Out", "In")
    
    # Transform -> Filters
    start_trans = nodes["Trans"]
    for f in nodes["Filters"]:
        try_connect(start_trans, f, "Out", "In")
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    
    # Sync
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def force_connect_hybrid():
    print(f"--- [Hybrid] Connect ---", flush=True)
    
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
    force_connect_hybrid()
