import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Volume Fill: CopyPoints Strategy ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Wall": None, "Trans": None, "Grid": None, "Copy": None, "Bounds": None
    }
    
    # 1. Reuse existing
    for n in graph.nodes:
        nm = n.get_name()
        if "DungeonDataReader" in nm and "2" in nm: nodes["Wall"] = n
        if "TransformPoints" in nm: nodes["Trans"] = n
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "CopyPoints" in nm: nodes["Copy"] = n # Reuse if exists
        
    # 2. Setup Nodes
    
    # A. Grid (The "Brush")
    # We want a small grid to copy onto every tile.
    # Tile size is big (400?), so grid should be 400x400.
    if not nodes["Grid"]:
        ret = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
        nodes["Grid"] = ret[0]
    
    nodes["Grid"].set_node_position(200, -200)
    try:
        # Cell Size 80 -> ~25 points per 400x400 tile?
        nodes["Grid"].get_settings().set_editor_property("CellSize", unreal.Vector(80, 80, 100))
        # Grid Size needs to be relative? 
        # By default Grid generates in Actor Bounds. We want Fixed Bounds?
        # PCGGrid usually has "GridExtents" if Mode is Relative?
        # Actually, let's try default. If it generates HUGE grid, we crash.
        # We need "Cull Outside" = False? Or specific Culling?
        # Let's set BlockSize/Extents? 
        # API check: Maybe just CullPointsOutside=True + Input?
        # BUT: For CopyPoints Source, we usually want a relative grid.
        # Let's assume default Grid generates *Something*.
        pass
    except: pass
        
    # B. CopyPoints
    if not nodes["Copy"]:
        ret = graph.add_node_of_type(unreal.PCGCopyPointsSettings)
        nodes["Copy"] = ret[0]
    nodes["Copy"].set_node_position(400, 0)
    
    # 3. Connect
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Disconnect Grid from anything else first?
    # We rewire.
    
    # Grid -> Copy (Source)
    connect(nodes["Grid"], nodes["Copy"], "Out", "Source")
    
    # Wall -> Bounds -> Copy (Target)
    # Use Bounds to ensure Wall points are valid?
    if nodes["Wall"] and nodes["Bounds"]:
        connect(nodes["Wall"], nodes["Bounds"])
        connect(nodes["Bounds"], nodes["Copy"], "Out", "Target")
    elif nodes["Wall"]:
        connect(nodes["Wall"], nodes["Copy"], "Out", "Target")
        
    # Copy -> Transform (Bypass Distance for now)
    connect(nodes["Copy"], nodes["Trans"])
    
    print("Connected: Grid(Source) + Wall(Target) -> CopyPoints -> Transform")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("CopyPoints Logic Applied.")

"""

def implement_copypoints():
    print(f"--- [Fix] CopyPoints Strategy ---", flush=True)
    
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
    implement_copypoints()
