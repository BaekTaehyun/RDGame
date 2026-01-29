import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Robust Reconnect V4 (Scope Fix) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Wall": None, "Floor": None, "Bounds": None, "Dist": None,
        "Trans": None, "Proj": None, "Grid": None,
        "F1": None, "F2": None, "F3": None, "F4": None,
        "S0": None, "S1": None, "S2": None, "S3": None
    }
    
    # 1. Identify Nodes
    filters = []
    
    for n in graph.nodes:
        nm = n.get_name()
        
        if "DungeonDataReader" in nm:
            t = ""
            try: t = str(n.get_editor_property("NodeTitleOverride"))
            except: pass
            
            if "Wall" in t: nodes["Wall"] = n
            elif "Floor" in t: nodes["Floor"] = n
            elif "2" in nm: 
                if not nodes["Wall"]: nodes["Wall"] = n
            elif "1" in nm:
                if not nodes["Floor"]: nodes["Floor"] = n
                
        elif "BoundsModifier" in nm: nodes["Bounds"] = n
        elif "Distance" in nm: nodes["Dist"] = n
        elif "TransformPoints" in nm: nodes["Trans"] = n
        elif "Projection" in nm: nodes["Proj"] = n
        elif "CreatePointsGrid" in nm: nodes["Grid"] = n
        
        elif "DensityFilter" in nm: filters.append(n)
        
        elif "StaticMeshSpawner" in nm:
            if "Spawner_0" in nm: nodes["S0"] = n
            elif "Spawner_1" in nm: nodes["S1"] = n
            elif "Spawner_2" in nm: nodes["S2"] = n
            elif "Spawner_3" in nm: nodes["S3"] = n

    filters.sort(key=lambda x: x.get_name())
    if len(filters) > 0: nodes["F1"] = filters[0]
    if len(filters) > 1: nodes["F2"] = filters[1]
    if len(filters) > 2: nodes["F3"] = filters[2]
    if len(filters) > 3: nodes["F4"] = filters[3]

    def connect(g, src, dst, sp="Out", dp="In"):
        if not src or not dst: return
        try:
            g.add_edge(src, sp, dst, dp)
            print(f"Conn: {src.get_name()} -> {dst.get_name()}")
        except Exception as e:
            print(f"Err: {src.get_name()}->{dst.get_name()}: {e}")

    # 2. Repair
    
    connect(graph, nodes["Wall"], nodes["Bounds"])
    
    target_for_dist = nodes["Bounds"]
    if nodes["Grid"]:
        connect(graph, nodes["Bounds"], nodes["Grid"], "Out", "In")
        target_for_dist = nodes["Grid"]
        
    if nodes["Dist"] and nodes["Floor"]:
        connect(graph, target_for_dist, nodes["Dist"], "Out", "Source")
        connect(graph, nodes["Floor"], nodes["Dist"], "Out", "Target")
        connect(graph, nodes["Dist"], nodes["Trans"])
    else:
        print("Bypassing Dist")
        connect(graph, target_for_dist, nodes["Trans"])

    connect(graph, nodes["Trans"], nodes["Proj"])
    
    if nodes["Proj"]:
        connect(graph, nodes["Proj"], nodes["F1"])
        connect(graph, nodes["Proj"], nodes["F2"])
        connect(graph, nodes["Proj"], nodes["F3"])
        connect(graph, nodes["Proj"], nodes["F4"])
        
    connect(graph, nodes["F1"], nodes["S0"])
    connect(graph, nodes["F2"], nodes["S1"])
    connect(graph, nodes["F3"], nodes["S2"])
    connect(graph, nodes["F4"], nodes["S3"])

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Reconnect V4 Done.")

"""

def robust_reconnect_v4():
    print(f"--- [Fix] Robust Reconnect V4 ---", flush=True)
    
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
    robust_reconnect_v4()
