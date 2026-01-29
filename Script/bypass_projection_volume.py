import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Debug] Volume Fill NO PROJECTION ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {}
    for n in graph.nodes:
        nm = n.get_name()
        if "DungeonDataReader" in nm and "2" in nm: nodes["Wall"] = n
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "StaticMeshSpawner" in nm and "Spawner_0" in nm: nodes["S0"] = n
        
    # Setup
    if not nodes["Grid"]:
        ret = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
        nodes["Grid"] = ret[0]
        
    # Reset Grid Settings to be safe
    # "Create Points Grid" from Input usually uses input bounds.
    # Set Cell Size 80.
    try:
        nodes["Grid"].get_settings().set_editor_property("CellSize", unreal.Vector(80, 80, 100))
        # Ensure 'Cull Points Outside' matches input? Default is True usually.
    except: pass
    
    # Connect Chain: Wall -> Bounds -> Grid -> Spawner_0
    # Bypass Transform, Distance, Projection, Filters.
    
    def connect(src, dst):
        try: graph.add_edge(src, "Out", dst, "In")
        except: pass
        
    connect(nodes["Wall"], nodes["Bounds"])
    connect(nodes["Bounds"], nodes["Grid"])
    
    # Direct to Spawner
    connect(nodes["Grid"], nodes["S0"])
    
    print("Connected: Wall -> Bounds -> Grid -> Spawner_0")
    print("REMOVED: Projection, Distance, Transform, CopyPoints.")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Test Configured.")

"""

def bypass_proj():
    print(f"--- [Debug] Testing Volume Fill (No Proj) ---", flush=True)
    
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
    bypass_proj()
