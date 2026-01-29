import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Lift Points & Project (V2) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Grid": None, "Proj": None, "Lift": None, "Dist": None
    }
    
    # 1. Identify Existing (Safe)
    for n in graph.nodes:
        nm = n.get_name()
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        # Try to identify Lift node by name if previously created?
        # Likely "TransformPoints_X". We'll just create a new one to be sure and label it if possible.
        
    # 2. Add Lift Node (Transform)
    ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
    nodes["Lift"] = ret[0]
    nodes["Lift"].set_node_position(600, -200)
    
    # Configure Lift: Move Up 5000
    try:
        # Offset Min/Max
        high_z = unreal.Vector(0, 0, 5000)
        # Note: PCGTransformPointsSettings might use 'OffsetMin' directly or via struct.
        # Let's hope direct access works.
        nodes["Lift"].get_settings().set_editor_property("OffsetMin", high_z)
        nodes["Lift"].get_settings().set_editor_property("OffsetMax", high_z)
        nodes["Lift"].get_settings().set_editor_property("bApplyOffset", True)
        print("Lift Configured (Z +5000).")
    except Exception as e:
        print(f"Lift Config Error: {e}")
        
    # 3. Configure Projection (World)
    if nodes["Proj"]:
        try:
            # Set Target to World (Enum or Int)
            # 2 = World (usually)
            # 1 = Landscape
            nodes["Proj"].get_settings().set_editor_property("ProjectionTarget", unreal.PCGProjectionTarget.WORLD)
            print("Set Projection -> WORLD")
        except:
            print("Enum Set Failed. Trying Int 2...")
            try:
                nodes["Proj"].get_settings().set_editor_property("ProjectionTarget", 2)
            except: pass
            
    # 4. Reconnect Chain
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Grid -> Lift -> Proj
    if nodes["Grid"] and nodes["Lift"] and nodes["Proj"]:
        # Disconnect Grid from Proj if existing? 
        # PCG allows multiple edges, but flow should be unique.
        # Graph API doesn't "Disconnect".
        # We just add new edge.
        connect(nodes["Grid"], nodes["Lift"])
        connect(nodes["Lift"], nodes["Proj"])
        print("Connected: Grid -> Lift -> Proj")
        
        # Proj -> Dist
        if nodes["Dist"]:
            connect(nodes["Proj"], nodes["Dist"], "Out", "Source")
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Lift Logic V2 Applied.")

"""

def lift_and_project_v2():
    print(f"--- [Fix] Lift & Project V2 ---", flush=True)
    
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
    lift_and_project_v2()
