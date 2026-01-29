import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Breaking Grid Pattern & Reducing Density ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Find the Grid Node
    grid_node = None
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0":
            grid_node = n
            break
            
    if not grid_node:
        print("Critical: Grid Node Not Found!")
    else:
        # 2. Check connections to see where to inject Transform
        # We want: Grid -> Transform -> [Existing Downstream]
        # Since we can't iterate edges easily, we assume Grid feeds 'CopyPoints_0' or Filters.
        # We will create a new Transform Node and place it near grid.
        
        # Check if we already created it
        trans_node = None
        for n in graph.nodes:
            title = "Unknown"
            try: title = n.get_editor_property("NodeTitleOverride").__str__()
            except: pass
            if "Forest_Transform" in title:
                trans_node = n
                break
        
        if not trans_node:
            ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
            trans_node = ret[0]
            try:
                trans_node.set_editor_property("NodeTitleOverride", "Forest_Transform")
                # Position
                px = grid_node.get_editor_property("NodePosX")
                py = grid_node.get_editor_property("NodePosY")
                trans_node.set_node_position(px + 200, py) # Move right
            except: pass
            
        # Configure Transform (JITTER is key)
        if trans_node:
            s = trans_node.get_settings()
            try:
                # Just set values. Booleans might be missing or defaulted.
                # If they are defaulted True, this works.
                # If defaulted False and inaccessible, we are in trouble, but let's try.
                
                # Rotation
                s.set_editor_property("RotationMin", unreal.Rotator(0, 0, 0))
                s.set_editor_property("RotationMax", unreal.Rotator(0, 360, 0))
                
                # Jitter (Position)
                jitter = 150.0 
                s.set_editor_property("OffsetMin", unreal.Vector(-jitter, -jitter, 0))
                s.set_editor_property("OffsetMax", unreal.Vector(jitter, jitter, 0))
                try: s.set_editor_property("AbsoluteOffset", True)
                except: pass
                
                # Scale
                s.set_editor_property("ScaleMin", unreal.Vector(0.8, 0.8, 0.8))
                s.set_editor_property("ScaleMax", unreal.Vector(1.4, 1.4, 1.4))
                try: s.set_editor_property("UniformScale", True)
                except: pass
                
                print("Forest_Transform configured (Values Only).")
            except Exception as e:
                print(f"Transform Settings Error: {e}")
                
            # Connect Grid -> Transform
            # Ideally we want to *Insert*, but simpler to just Branch for now.
            # If we branch, the old connection (Grid -> Raw) remains?
            # Yes. Unwanted.
            # We must BREAK connections from Grid.
            # Since we can't easily break specific edges in Python without iterating pins,
            # this is hard.
            # BUT, we can Connect Grid -> Transform.
            try:
                graph.add_edge(grid_node, "Out", trans_node, "In")
                print("Connected: Grid -> Forest_Transform")
                
                # Now connect Transform to 'CopyPoints_0' which distributes to others? 
                # Or Density Filters?
                # Topology said 'CopyPoints_0' is at same location.
                # Let's connect Transform to ALL Density Filters (1-4).
                # This ensures they get the jittered points.
                
                filters = ["DensityFilter_1", "DensityFilter_2", "DensityFilter_3", "DensityFilter_4"]
                for fname in filters:
                     found_f = None
                     for n in graph.nodes:
                         if n.get_name() == fname:
                             found_f = n
                             break
                     if found_f:
                         graph.add_edge(trans_node, "Out", found_f, "In")
                         print(f"Connected: Forest_Transform -> {fname}")
                         
            except Exception as e:
                print(f"Connection Error: {e}")

    # 3. Thin ALL Layers (Big Trees too)
    filters = ["DensityFilter_1", "DensityFilter_2", "DensityFilter_3", "DensityFilter_4"]
    for fname in filters:
        for n in graph.nodes:
            if n.get_name() == fname:
                try:
                    s = n.get_settings()
                    # 0.75 is a good baseline for "Sparse but present"
                    try: s.lower_bound = 0.75
                    except: s.set_editor_property("LowerBound", 0.75)
                    print(f"Set {fname} LowerBound to 0.75")
                except: pass

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Pattern Broken & Density Reset.")

"""

def fix_pattern():
    print(f"--- [Fix] Breaking Patterns ---", flush=True)
    
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
    fix_pattern()
