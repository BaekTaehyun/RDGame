import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Full Rewire (V2) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {}
    filters = []
    spawners = []
    
    # 1. Identify Nodes
    for n in graph.nodes:
        nm = n.get_name()
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        if "DungeonDataReader" in nm:
            try:
                t = n.get_editor_property("NodeTitleOverride")
                if "Wall" in t: nodes["Wall"] = n
                elif "Floor" in t: nodes["Floor"] = n
            except: 
                if "2" in nm: nodes["Wall"] = n
                if "1" in nm: nodes["Floor"] = n

        if "TransformPoints" in nm:
            try:
                off = n.get_settings().get_editor_property("OffsetMin")
                if off.z > 500: nodes["Lift"] = n
                else: nodes["Trans"] = n
            except: pass
            
        if "DensityFilter" in nm: filters.append(n)
        if "StaticMeshSpawner" in nm: spawners.append(n)
        
    filters.sort(key=lambda x: x.get_name())
    spawners.sort(key=lambda x: x.get_name())
    for i in range(4):
        if i < len(filters): nodes[f"F{i+1}"] = filters[i]
        if i < len(spawners): nodes[f"S{i}"] = spawners[i]

    # 2. Fix Grid Settings
    if nodes["Grid"]:
        try:
            s = nodes["Grid"].get_settings()
            # Try Enum Value (unreal.PCGCoordinateSpace.RELATIVE)?
            # Or iterate enums of property.
            # Usually we can set integer if we wrap it in Enum?
            # But TypeError said "Cannot nativize int as EnumProperty".
            # Try setting as String? "Relative"?
            # Or create Enum type if available. Note: 'unreal.PCGCoordinateSpace' might fail if not exposed.
            # PCGCoordinateSpace: 0=Global, 1=Local/Relative?
            # Let's try string "Relative".
            try:
                s.set_editor_property("CoordinateSpace", unreal.PCGCoordinateSpace.RELATIVE)
                print("Set Relative (Enum)")
            except:
                print("PCGCoordinateSpace Enum not found. Trying string 'Relative'...")
                try: s.set_editor_property("CoordinateSpace", "Relative")
                except: pass
                
            # Extents
            ext = unreal.Vector(250, 250, 50)
            s.set_editor_property("GridExtents", ext)
            print("Set Grid Extents.")
        except Exception as e:
            print(f"Grid Settings Error: {e}")

    # 3. Rewire
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Wall -> Bounds -> Grid -> Lift -> Proj -> Dist
    if nodes.get("Wall") and nodes.get("Bounds"): connect(nodes["Wall"], nodes["Bounds"])
    if nodes.get("Bounds") and nodes.get("Grid"): connect(nodes["Bounds"], nodes["Grid"])
    if nodes.get("Grid") and nodes.get("Lift"): connect(nodes["Grid"], nodes["Lift"])
    if nodes.get("Lift") and nodes.get("Proj"): connect(nodes["Lift"], nodes["Proj"])
    
    # Proj -> Dist (Source) | Floor -> Dist (Target)
    if nodes.get("Proj") and nodes.get("Dist"): connect(nodes["Proj"], nodes["Dist"], "Out", "Source")
    if nodes.get("Floor") and nodes.get("Dist"): connect(nodes["Floor"], nodes["Dist"], "Out", "Target")
    
    # Dist -> Trans -> Filters
    if nodes.get("Dist") and nodes.get("Trans"): connect(nodes["Dist"], nodes["Trans"])
    
    src = nodes.get("Trans")
    if src:
        # F1->S0
        if nodes.get("F1"):
            connect(src, nodes["F1"])
            if nodes.get("S0"): connect(nodes["F1"], nodes["S0"])
        # F2->S1
        if nodes.get("F2"):
            connect(src, nodes["F2"])
            if nodes.get("S1"): connect(nodes["F2"], nodes["S1"])
        # F3->S2
        if nodes.get("F3"):
            connect(src, nodes["F3"])
            if nodes.get("S2"): connect(nodes["F3"], nodes["S2"])
        # F4->S3
        if nodes.get("F4"):
            connect(src, nodes["F4"])
            if nodes.get("S3"): connect(nodes["F4"], nodes["S3"])
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Rewire V2 Complete.")

    # 4. Refresh Editor
    asset_sub = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
    # Just try asking if it is open, if so close/open
    # close_all_editors_for_asset takes asset object
    try:
        asset_sub.close_all_editors_for_asset(graph)
        asset_sub.open_editor_for_asset(graph)
        print("Editor Refreshed.")
    except Exception as e:
        print(f"Editor Refresh Error: {e}")

"""

def rewire_v2():
    print(f"--- [Fix] Rewire V2 ---", flush=True)
    
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
    rewire_v2()
