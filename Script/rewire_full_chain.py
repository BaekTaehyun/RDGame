import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Full Rewire: Wall->Bounds->Grid(Relative) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Wall": None, "Floor": None, "Bounds": None, "Grid": None,
        "Lift": None, "Proj": None, "Dist": None, "Trans": None,
        "F1": None, "F2": None, "F3": None, "F4": None,
        "S0": None, "S1": None, "S2": None, "S3": None
    }
    
    # 1. Identify Nodes
    filters = []
    spawners = []
    
    for n in graph.nodes:
        nm = n.get_name()
        
        # Reader Identification
        if "DungeonDataReader" in nm:
            title = ""
            try: title = n.get_editor_property("NodeTitleOverride")
            except: pass
            if "Wall" in title: nodes["Wall"] = n
            elif "Floor" in title: nodes["Floor"] = n
            else:
                if "2" in nm: nodes["Wall"] = n
                if "1" in nm: nodes["Floor"] = n
                
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        
        # Lift is the TransformPoints with high Z
        if "TransformPoints" in nm:
            try:
                s = n.get_settings()
                off = s.get_editor_property("OffsetMin")
                if off.z > 500: nodes["Lift"] = n
                else: nodes["Trans"] = n # The randomization transform
            except: pass
            
        if "DensityFilter" in nm: filters.append(n)
        if "StaticMeshSpawner" in nm: spawners.append(n)
        
    # Sort Filters/Spawners
    filters.sort(key=lambda x: x.get_name())
    spawners.sort(key=lambda x: x.get_name())
    
    for i, f in enumerate(filters):
        if i < 4: nodes[f"F{i+1}"] = f
    for i, s in enumerate(spawners):
        if i < 4: nodes[f"S{i}"] = s # S0, S1, S2, S3
        
    # 2. Fix Grid Settings (Relative Space)
    if nodes["Grid"]:
        try:
            s = nodes["Grid"].get_settings()
            # CoordinateSpace Enum: 0=Global, 1=Relative? (Guessing)
            # Actually standard is: Global=0, Local=1?
            # Let's try to set to Relative (1).
            s.set_editor_property("CoordinateSpace", 1) # 1 = Relative usually
            print("Grid CoordinateSpace -> Relative (1)")
            
            # Set Extents 250
            ext = unreal.Vector(250, 250, 50)
            s.set_editor_property("GridExtents", ext)
            print("Grid Extents -> 250")
        except Exception as e:
            print(f"Grid Settings Error: {e}")
            
    # 3. Rewire Everything
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Chain A: Wall -> Bounds -> Grid -> Lift -> Proj
    if nodes["Wall"] and nodes["Bounds"]: connect(nodes["Wall"], nodes["Bounds"])
    if nodes["Bounds"] and nodes["Grid"]: connect(nodes["Bounds"], nodes["Grid"]) # CRITICAL FIX
    if nodes["Grid"] and nodes["Lift"]: connect(nodes["Grid"], nodes["Lift"])
    if nodes["Lift"] and nodes["Proj"]: connect(nodes["Lift"], nodes["Proj"])
    
    # Chain B: (Proj + Floor) -> Distance -> Trans
    if nodes["Proj"] and nodes["Dist"]: connect(nodes["Proj"], nodes["Dist"], "Out", "Source")
    if nodes["Floor"] and nodes["Dist"]: connect(nodes["Floor"], nodes["Dist"], "Out", "Target")
    if nodes["Dist"] and nodes["Trans"]: connect(nodes["Dist"], nodes["Trans"])
    
    # Chain C: Trans -> Filters -> Spawners
    # Filter 1 (Big) -> Spawner 0 (Big)
    src = nodes["Trans"]
    if src:
        if nodes["F1"]:
            connect(src, nodes["F1"])
            if nodes["S0"]: connect(nodes["F1"], nodes["S0"])
            
        if nodes["F2"]:
            connect(src, nodes["F2"])
            if nodes["S1"]: connect(nodes["F2"], nodes["S1"])
            
        if nodes["F3"]:
            connect(src, nodes["F3"])
            if nodes["S2"]: connect(nodes["F3"], nodes["S2"])
            
        if nodes["F4"]:
            connect(src, nodes["F4"])
            if nodes["S3"]: connect(nodes["F4"], nodes["S3"])
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Full Rewire Completed.")
    
    # 4. Attempt Editor Refresh
    # Close and Reopen Asset Editor?
    asset_sub = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
    if asset_sub.find_editor_for_asset(graph):
        asset_sub.close_all_editors_for_asset(graph)
        asset_sub.open_editor_for_asset(graph)
        print("Re-opened Editor to Sync.")

"""

def rewire_chain():
    print(f"--- [Fix] Rewire Full ---", flush=True)
    
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
    rewire_chain()
