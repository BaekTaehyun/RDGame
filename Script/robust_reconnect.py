import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Robust Reconnect (Pin Inspection) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Nodes
    nodes = {
        "Wall": None,
        "Floor": None,
        "Bounds": None,
        "Dist": None,
        "Trans": None,
        "Proj": None,
        "F1": None, "F2": None, "F3": None, "F4": None,
        "S0": None, "S1": None, "S2": None, "S3": None
    }
    
    for n in graph.nodes:
        nm = n.get_name()
        # Wall/Floor
        if "DungeonDataReader" in nm:
            t = str(n.get_editor_property("NodeTitleOverride"))
            if "Wall" in t: nodes["Wall"] = n
            elif "Floor" in t: nodes["Floor"] = n
            elif "2" in nm: nodes["Wall"] = n # Fallback
            elif "1" in nm: nodes["Floor"] = n
            
        elif "BoundsModifier" in nm: nodes["Bounds"] = n
        elif "Distance" in nm: nodes["Dist"] = n
        elif "TransformPoints" in nm: nodes["Trans"] = n
        elif "Projection" in nm: nodes["Proj"] = n
        
        # Filters (Position based sorting?)
        elif "DensityFilter" in nm:
            # We assume creation order or position. 
            # Sort generic collection?
            pass
            
        elif "StaticMeshSpawner" in nm:
            if "Spawner_0" in nm: nodes["S0"] = n
            elif "Spawner_1" in nm: nodes["S1"] = n
            elif "Spawner_2" in nm: nodes["S2"] = n
            elif "Spawner_3" in nm: nodes["S3"] = n

    # Collect filters by position height to assign F1..F4
    filters = []
    for n in graph.nodes:
        if "DensityFilter" in n.get_name():
            filters.append(n)
    # Sort Y position: -200, 0, 200, 400
    filters.sort(key=lambda x: x.node_position_y)
    
    if len(filters) >= 4:
        nodes["F1"] = filters[0]
        nodes["F2"] = filters[1]
        nodes["F3"] = filters[2]
        nodes["F4"] = filters[3]
    elif len(filters) > 0:
        nodes["F1"] = filters[0] # Fallback

    # 2. Helper Logic: Get Pin Name
    def connect(src, dst, src_pin_idx=0, dst_pin_idx=0, dynamic_names=None):
        if not src or not dst: return
        
        # Try to find pin names. 
        # PCG Python API for pins is tricky. 'add_edge' uses names.
        # Commonly "Out", "In". But sometimes "Output", "Input".
        # Distance node inputs: "Source", "Target".
        
        s_name = "Out"
        d_name = "In"
        
        if dynamic_names:
            s_name = dynamic_names[0]
            d_name = dynamic_names[1]
            
        try:
            graph.add_edge_by_name(src, s_name, dst, d_name)
        except:
            try:
                graph.add_edge(src, s_name, dst, d_name)
            except Exception as e:
                print(f"Failed {src.get_name()}->{dst.get_name()}: {e}")

    # 3. Execution Reconnection
    
    # Wall -> Bounds
    connect(nodes["Wall"], nodes["Bounds"])
    
    # Bounds -> Dist ("Source")
    # Floor -> Dist ("Target")
    if nodes["Dist"]:
        # Standard Distance Pins: Source, Target. Output: Out.
        # But 'add_edge' usually just takes default 'Out' to 'In'.
        # For Distance, we explicitly need Source/Target.
        # User screenshot shows Distance inputs as "Source", "Target".
        
        # Method: add_edge_by_name IS NOT AVAILABLE (Previous Error).
        # We must use add_edge with pin names. 
        # Wait, 'add_edge(upstream, downstream)' connects DEFAULT pins?
        # If we need specific pins, we might be stuck if add_edge_by_name is gone.
        # BUT: 'add_edge' signature: (InputNode, InputPinName, OutputNode, OutputPinName)?
        # No, 'add_edge(UpstreamNode, UpstreamPinLabel, DownstreamNode, DownstreamPinLabel)'
        
        try:
            graph.add_edge(nodes["Bounds"], "Out", nodes["Dist"], "Source")
            graph.add_edge(nodes["Floor"], "Out", nodes["Dist"], "Target")
            print("Connected Distance Inputs.")
        except Exception as e:
            print(f"Dist Connect Error: {e}")

        # Dist -> Trans
        connect(nodes["Dist"], nodes["Trans"])
    else:
        # Fallback Bypass Dist
        connect(nodes["Bounds"], nodes["Trans"])

    # Trans -> Proj
    connect(nodes["Trans"], nodes["Proj"])
    
    # Proj -> Filters
    if nodes["Proj"]:
        if nodes["F1"]: connect(nodes["Proj"], nodes["F1"])
        if nodes["F2"]: connect(nodes["Proj"], nodes["F2"])
        if nodes["F3"]: connect(nodes["Proj"], nodes["F3"])
        if nodes["F4"]: connect(nodes["Proj"], nodes["F4"])
        
    # Filters -> Spawners
    if nodes["F1"] and nodes["S0"]: connect(nodes["F1"], nodes["S0"])
    if nodes["F2"] and nodes["S1"]: connect(nodes["F2"], nodes["S1"])
    if nodes["F3"] and nodes["S2"]: connect(nodes["F3"], nodes["S2"])
    if nodes["F4"] and nodes["S3"]: connect(nodes["F4"], nodes["S3"])

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Reconnection Attempted.")

"""

def robust_reconnect():
    print(f"--- [Fix] Robust Reconnect ---", flush=True)
    
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
    robust_reconnect()
