import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Restore Projection & Ecotone ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Nodes
    nodes = {
        "Wall": None, "Floor": None, "Bounds": None, "Grid": None,
        "Proj": None, "Dist": None, "Trans": None,
        "F1": None, "F2": None, "F3": None, "F4": None
    }
    
    for n in graph.nodes:
        nm = n.get_name()
        if "DungeonDataReader" in nm and "2" in nm: nodes["Wall"] = n
        if "DungeonDataReader" in nm and "1" in nm: nodes["Floor"] = n # Assuming 1 is Floor
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        if "TransformPoints" in nm: nodes["Trans"] = n
        
    # Helpers for Filters
    filters = []
    for n in graph.nodes:
        if "DensityFilter" in n.get_name(): filters.append(n)
    filters.sort(key=lambda x: x.get_name())
    if len(filters) > 3:
        nodes["F1"] = filters[0]
        nodes["F2"] = filters[1]
        nodes["F3"] = filters[2]
        nodes["F4"] = filters[3]
        
    # 2. Tune Grid (Reduce Density)
    if nodes["Grid"]:
        try:
            # 80 was too dense (solid blob). Try 150.
            nodes["Grid"].get_settings().set_editor_property("CellSize", unreal.Vector(150, 150, 200))
        except: pass
        
    # 3. Connection Helper
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass
        
    # 4. Rebuild Chain
    
    # A. Wall -> Bounds -> Grid
    connect(nodes["Wall"], nodes["Bounds"])
    connect(nodes["Bounds"], nodes["Grid"])
    
    # B. Grid -> Projection (Snap floating blob to ground)
    # Note: If Projection is "WorldRayHit", it needs "Project Target"? 
    # Usually Projection defaults works on Landscape.
    if nodes["Proj"]:
        connect(nodes["Grid"], nodes["Proj"])
        
        # C. Projection -> Distance (Source)
        if nodes["Dist"]:
            connect(nodes["Proj"], nodes["Dist"], "Out", "Source")
            if nodes["Floor"]:
                connect(nodes["Floor"], nodes["Dist"], "Out", "Target")
            
            # D. Distance -> Transform
            connect(nodes["Dist"], nodes["Trans"])
            
            # E. Transform -> Filters
            # If Filters connected to Spawners already, we just feed Filters.
            # Wait, Filters need Input from Trans.
            src_node = nodes["Trans"]
            
            if nodes["F1"]: connect(src_node, nodes["F1"])
            if nodes["F2"]: connect(src_node, nodes["F2"])
            if nodes["F3"]: connect(src_node, nodes["F3"])
            if nodes["F4"]: connect(src_node, nodes["F4"])
            
            print("Connected: Grid -> Proj -> Dist -> Trans -> Filters")
        else:
            print("Distance Node Missing!")
    else:
        print("Projection Node Missing!")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Logic Restored.")

"""

def restore_logic():
    print(f"--- [Fix] Restore Logic ---", flush=True)
    
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
    restore_logic()
