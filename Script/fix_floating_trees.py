import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Lift & Snap (Floating Trees) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Grid": None, "Proj": None, "Lift": None, "Dist": None
    }
    
    # 1. Find Nodes
    # We look for a Transform node that is NOT the main randomization transform.
    # We call it "LiftTransform".
    transform_nodes = []
    for n in graph.nodes:
        nm = n.get_name()
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        if "TransformPoints" in nm: transform_nodes.append(n)
        
    # Check if we already have a Lift node (created in previous failed attempts)
    # We'll just create a fresh one to be safe and ensure logic.
    
    ret = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
    nodes["Lift"] = ret[0]
    nodes["Lift"].set_node_position(600, -200)
    
    # 2. Config Lift (Z + 10,000)
    try:
        high_z = unreal.Vector(0, 0, 10000)
        s = nodes["Lift"].get_settings()
        s.set_editor_property("OffsetMin", high_z)
        s.set_editor_property("OffsetMax", high_z)
        
        # Try to enable Apply (defaults are usually True)
        try: s.set_editor_property("bApplyTransform", True)
        except: pass
        
        print("Lift Configured: Z+10000")
    except Exception as e:
        print(f"Lift Error: {e}")
        
    # 3. Config Projection
    if nodes["Proj"]:
        try:
            # Set to World (Index 2 usually, or Enum)
            nodes["Proj"].get_settings().set_editor_property("ProjectionTarget", unreal.PCGProjectionTarget.WORLD)
            print("Projection -> WORLD")
        except:
            # Fallback
            try: nodes["Proj"].get_settings().set_editor_property("ProjectionTarget", 2)
            except: pass

    # 4. Reconnect Chain
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    if nodes["Grid"] and nodes["Lift"] and nodes["Proj"]:
        # Grid -> Lift -> Proj
        connect(nodes["Grid"], nodes["Lift"])
        connect(nodes["Lift"], nodes["Proj"])
        print("Chain: Grid -> Lift -> Proj")
        
        if nodes["Dist"]:
            connect(nodes["Proj"], nodes["Dist"], "Out", "Source")
            
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Logic Updated.")

"""

def fix_floating():
    print(f"--- [Fix] Fix Floating ---", flush=True)
    
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
    fix_floating()
