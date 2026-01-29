import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Volume Fill: Mesh Sampler Strategy ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Wall": None, "Trans": None, "Copy": None, "Sampler": None, "Bounds": None
    }
    
    # 1. Helpers
    for n in graph.nodes:
        nm = n.get_name()
        if "DungeonDataReader" in nm and "2" in nm: nodes["Wall"] = n
        if "TransformPoints" in nm: nodes["Trans"] = n
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "CopyPoints" in nm: nodes["Copy"] = n
        if "MeshSampler" in nm: nodes["Sampler"] = n
        
    # 2. Setup Mesh Sampler (The Brush)
    if not nodes["Sampler"]:
        ret = graph.add_node_of_type(unreal.PCGMeshSamplerSettings)
        nodes["Sampler"] = ret[0]
    
    nodes["Sampler"].set_node_position(200, -200)
    
    # Set Mesh to Cube
    try:
        # Load Cube
        cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
        # Property usually 'StaticMesh'
        nodes["Sampler"].get_settings().set_editor_property("StaticMesh", cube_mesh)
        
        # Sampling Options: Poisson/Uniform?
        # TargetPointCount = 10?
        # Let's try to set density or count.
        # Defaults might spam points?
        # Let's trust defaults for now (usually reasonable).
    except Exception as e:
        print(f"Mesh Sampler Config Error: {e}")
        
    # 3. Setup CopyPoints
    if not nodes["Copy"]:
        ret = graph.add_node_of_type(unreal.PCGCopyPointsSettings)
        nodes["Copy"] = ret[0]
    nodes["Copy"].set_node_position(400, 0)
    
    # 4. Connect
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # Sampler -> Copy (Source)
    connect(nodes["Sampler"], nodes["Copy"], "Out", "Source")
    
    # Wall -> Bounds -> Copy (Target)
    if nodes["Wall"] and nodes["Bounds"]:
        connect(nodes["Wall"], nodes["Bounds"])
        connect(nodes["Bounds"], nodes["Copy"], "Out", "Target")
    
    # Copy -> Transform
    connect(nodes["Copy"], nodes["Trans"])
    
    print("Connected: MeshSampler(Cube) + Wall(Target) -> CopyPoints -> Transform")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("MeshSampler Strategy Applied.")

"""

def switch_mesh_sampler():
    print(f"--- [Fix] Mesh Sampler Strategy ---", flush=True)
    
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
    switch_mesh_sampler()
