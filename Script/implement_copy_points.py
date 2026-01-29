import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Implementing Copy Points Strategy ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {}
    
    # 1. Identify Existing Nodes
    for n in graph.nodes:
        nm = n.get_name()
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "DungeonDataReader" in nm:
             try:
                 t = n.get_editor_property("NodeTitleOverride")
                 if "Wall" in t: nodes["Wall"] = n
             except:
                 if "2" in nm: nodes["Wall"] = n
                 
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        
        # Downstream start (Lift)
        if "TransformPoints" in nm:
             try:
                 off = n.get_settings().get_editor_property("OffsetMin")
                 if off.z > 500: nodes["Lift"] = n
             except: pass

    # 2. Create Copy Points Node
    if nodes.get("Grid") and nodes.get("Wall"):
        copy_node = None
        # Check if already exists to avoid dupes
        for n in graph.nodes:
            if "CopyPoints" in n.get_name():
                copy_node = n
                break
        
        if not copy_node:
            ret = graph.add_node_of_type(unreal.PCGCopyPointsSettings)
            copy_node = ret[0]
            copy_node.set_node_position(400, 0) # Position between Grid/Wall and Lift
            print("Created CopyPoints Node.")
            
        nodes["Copy"] = copy_node

    # 3. Rewire
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    if nodes.get("Copy"):
        # Source: Grid (The Stamp)
        if nodes.get("Grid"):
            connect(nodes["Grid"], nodes["Copy"], "Out", "Source")
            print("Connected Grid -> Copy(Source)")
            
        # Target: Wall (The Locations)
        if nodes.get("Wall"):
            connect(nodes["Wall"], nodes["Copy"], "Out", "Target")
            print("Connected Wall -> Copy(Target)")
            
        # Output: Copy -> Lift
        if nodes.get("Lift"):
            connect(nodes["Copy"], nodes["Lift"], "Out", "In")
            print("Connected Copy -> Lift")
            
    # 4. Clean up Old Connections (Optional but good)
    # Ideally we'd remove edge Wall->Bounds or Bounds->Grid
    # But for now, ensuring the new path exists is key. 
    # The Grid might still have 'Bounds' input attached?
    # If Grid has input, it might behave differently.
    # To be safe, we can disconnect Bounds->Grid if possible?
    # graph.remove_edge(src, dst)? Not exposed easily.
    # But usually 'Grid' ignores input if we treat it as Source for Copy?
    
    # 5. Tunings
    # Ensure Grid Extents are small (240)
    if nodes.get("Grid"):
        s = nodes["Grid"].get_settings()
        ext = unreal.Vector(250, 250, 100)
        s.set_editor_property("GridExtents", ext)
        # Coordinate Space: Local/Relative is best for Stamp
        try: s.set_editor_property("CoordinateSpace", 1) # Relative
        except: pass
        print("Tuned Grid (250x250).")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Logic Implemented: Grid+Wall -> Copy -> Lift")

"""

def implement_copy_points():
    print(f"--- [Fix] Copy Points ---", flush=True)
    
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
    implement_copy_points()
