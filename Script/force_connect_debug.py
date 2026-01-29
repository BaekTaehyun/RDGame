import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Debug] Force Connect & Pin Inspection ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {}
    
    # 1. Map Nodes
    for n in graph.nodes:
        nm = n.get_name()
        # CreatePointsGrid
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        # CopyPoints
        if "CopyPoints" in nm: nodes["Copy"] = n
        # Readers
        if "DungeonDataReader" in nm:
            t = ""
            try: t = n.get_editor_property("NodeTitleOverride")
            except: pass
            if "Wall" in t or "2" in nm: nodes["Wall"] = n
            if "Floor" in t or "1" in nm: nodes["Floor"] = n
            
        # Lift (TransformPoints with Z=2000)
        if "TransformPoints" in nm:
            try:
                s = n.get_settings()
                off = s.get_editor_property("OffsetMin")
                if off.z > 500: nodes["Lift"] = n
                else: nodes["Trans"] = n # Randomizer
            except: pass
            
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        
        # Filters (Just get the first one to test chain)
        if "DensityFilter" in nm and "F1" not in nodes:
            nodes["F1"] = n
            
    # 2. Inspect Pins & Connect
    def inspect_and_connect(src_key, dst_key, src_pin_label, dst_pin_label):
        src = nodes.get(src_key)
        dst = nodes.get(dst_key)
        
        if not src:
            print(f"Missing Source Node: {src_key}")
            return
        if not dst:
            print(f"Missing Dest Node: {dst_key}")
            return
            
        # Inspect Pins
        print(f"\\nConnecting {src.get_name()} -> {dst.get_name()}")
        
        # Check Output Pins on Source
        found_src_pin = False
        for p in src.get_output_pins():
            print(f"  Src Pin: '{p.properties.label}'")
            if str(p.properties.label) == src_pin_label:
                found_src_pin = True
                
        # Check Input Pins on Dest
        found_dst_pin = False
        for p in dst.get_input_pins():
            print(f"  Dst Pin: '{p.properties.label}'")
            if str(p.properties.label) == dst_pin_label:
                found_dst_pin = True
                
        if not found_src_pin:
             print(f"  [Warn] Source Pin '{src_pin_label}' NOT FOUND on {src.get_name()}")
        if not found_dst_pin:
             print(f"  [Warn] Dest Pin '{dst_pin_label}' NOT FOUND on {dst.get_name()}")
             
        # Attempt Connect
        try:
            # Note: add_edge takes (UpstreamNode, UpstreamPinLabel, DownstreamNode, DownstreamPinLabel)
            # Ensure labels are explicitly names if needed, but strings usually work.
            graph.add_edge(src, src_pin_label, dst, dst_pin_label)
            print(f"  > Connected: {src_pin_label} -> {dst_pin_label}")
        except Exception as e:
            print(f"  [ERROR] Connect Failed: {e}")

    # CHAIN
    # 1. Grid -> Copy (Source)
    inspect_and_connect("Grid", "Copy", "Out", "Source")
    
    # 2. Wall -> Copy (Target)
    inspect_and_connect("Wall", "Copy", "Out", "Target")
    
    # 3. Copy -> Lift (Out -> In)
    inspect_and_connect("Copy", "Lift", "Out", "In")
    
    # 4. Lift -> Proj (Out -> In)
    inspect_and_connect("Lift", "Proj", "Out", "In")
    
    # 5. Proj -> Dist (Out -> Source)
    inspect_and_connect("Proj", "Dist", "Out", "Source")
    
    # 6. Floor -> Dist (Out -> Target)
    inspect_and_connect("Floor", "Dist", "Out", "Target")
    
    # 7. Dist -> Trans (Out -> In)
    inspect_and_connect("Dist", "Trans", "Out", "In")
    
    # 8. Trans -> F1
    inspect_and_connect("Trans", "F1", "Out", "In")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    
    # Sync
    try:
        unreal.DungeonAssetUtils.refresh_blueprint(graph)
        print("Sync Called.")
    except: pass
"""

def force_connect():
    print(f"--- [Debug] Force Connect ---", flush=True)
    
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
    force_connect()
