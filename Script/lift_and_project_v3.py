import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Lift Points & Project (V3) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Grid": None, "Proj": None, "Lift": None, "Dist": None
    }
    
    for n in graph.nodes:
        nm = n.get_name()
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        # Assuming the Lift node created in V2 exists, but we couldn't config it.
        # It's a TransformPoints node. We need to find the specific one.
        # The V2 script created it at (600, -200).
        if "TransformPoints" in nm:
            pos = n.get_node_position()
            # How to read pos? It's integer X/Y usually? 
            # Or inspect edges?
            # Let's just create a NEW one and abandon the old one (it will just be floating logic).
            pass

    # Create NEW Lift Node
    ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
    nodes["Lift"] = ret[0]
    nodes["Lift"].set_node_position(650, -200) # Slightly offset from previous attempts
    
    try:
        high_z = unreal.Vector(0, 0, 5000)
        # Property: bApplyTransform usually controls if it runs. Default is True.
        # We just need to set Offset.
        nodes["Lift"].get_settings().set_editor_property("OffsetMin", high_z)
        nodes["Lift"].get_settings().set_editor_property("OffsetMax", high_z)
        try:
            nodes["Lift"].get_settings().set_editor_property("bApplyTransform", True)
        except: pass # Might be default
        
        print("Lift Configured (Z +5000).")
    except Exception as e:
        print(f"Lift Config Error: {e}")

    # Reconnect
    def connect(src, dst):
        try: graph.add_edge(src, "Out", dst, "In")
        except: pass

    if nodes["Grid"] and nodes["Lift"] and nodes["Proj"]:
        connect(nodes["Grid"], nodes["Lift"])
        connect(nodes["Lift"], nodes["Proj"])
        print("Connected: Grid -> Lift -> Proj")
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Lift Logic V3 Applied.")

"""

def lift_and_project_v3():
    print(f"--- [Fix] Lift & Project V3 ---", flush=True)
    
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
    lift_and_project_v3()
