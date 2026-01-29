import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Lift Points & Project World ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Grid": None, "Proj": None, "Lift": None, "Dist": None
    }
    
    # 1. Identify Existing
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_name(): nodes["Grid"] = n
        if "Projection" in n.get_name(): nodes["Proj"] = n
        if "Distance" in n.get_name(): nodes["Dist"] = n
        if "LiftTransform" in n.get_editor_property("NodeTitleOverride"): nodes["Lift"] = n
        
    # 2. Add Lift Node (Transform)
    if not nodes["Lift"]:
        # Check if we have a spare transform we can repurpose or create new
        ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
        nodes["Lift"] = ret[0]
        nodes["Lift"].set_editor_property("NodeTitleOverride", "LiftTransform")
        nodes["Lift"].set_node_position(600, -200) # Between Grid(400) and Proj(1000)
    
    # Configure Lift: Move Up 5000
    try:
        # Check if 'OffsetMin'/'OffsetMax' exist or if it uses 'ApplyTransform'.
        # Usually TransformPoints has Min/Max relative.
        # We want Absolute Offset? Or Relative?
        # Relative Z + 5000.
        high_z = unreal.Vector(0, 0, 5000)
        nodes["Lift"].get_settings().set_editor_property("OffsetMin", high_z)
        nodes["Lift"].get_settings().set_editor_property("OffsetMax", high_z) # No Randomness, just Lift
        nodes["Lift"].get_settings().set_editor_property("bApplyOffset", True)
    except Exception as e:
        print(f"Lift Config Error: {e}")
        
    # 3. Configure Projection (World)
    if nodes["Proj"]:
        try:
            # Set Target to World (usually index 2 or enum)
            # 0=Blueprint, 1=Landscape, 2=World?
            # Let's try to set to 'Landscape' (1) and 'World' (2)
            # Actually, standard is 'Landscape' (1). If that failed, maybe 'World' (2).
            # Let's try to set by Int.
            # Warning: Property might be enum.
            nodes["Proj"].get_settings().set_editor_property("ProjectionTarget", unreal.PCGProjectionTarget.WORLD)
            print("Set Projection -> WORLD")
        except:
            print("Could not set WORLD target. Trying Landscape...")
            try:
                nodes["Proj"].get_settings().set_editor_property("ProjectionTarget", unreal.PCGProjectionTarget.LANDSCAPE)
            except: pass
            
    # 4. Reconnect Chain
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Grid -> Lift -> Proj
    if nodes["Grid"] and nodes["Lift"] and nodes["Proj"]:
        connect(nodes["Grid"], nodes["Lift"])
        connect(nodes["Lift"], nodes["Proj"])
        print("Connected: Grid -> Lift -> Proj")
        
        # Proj -> Dist (Existing chain)
        if nodes["Dist"]:
            connect(nodes["Proj"], nodes["Dist"], "Out", "Source")
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Lift Logic Applied.")

"""

def lift_and_project():
    print(f"--- [Fix] Lift & Project ---", flush=True)
    
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
    lift_and_project()
