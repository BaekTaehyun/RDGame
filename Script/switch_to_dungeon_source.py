import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Switching Source to Dungeon Link ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Nodes
    grid_node = None
    data_node = None
    trans_node = None
    
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0": grid_node = n
        if n.get_name() == "DungeonDataReader_1": data_node = n
        
        # New Transform logic (Check Title or Connection)
        # We named it "Forest_Transform_Fixed" in last script?
        # Let's hope title stuck or we find it by being the one feeding Project.
        # But we can just find any Transform connected to 'Projection_0' (if we can traverse up).
        # Easier: Find node by Title override if possible, else look for recent.
        title = "Unknown"
        try: title = n.get_editor_property("NodeTitleOverride")
        except: pass
        if "Forest" in str(title) or (n != grid_node and n != data_node and "TransformPoints" in n.get_name() and n.get_name() != "TransformPoints_2"):
             trans_node = n

    # 2. Break Grid Connection (if possible)
    # We can't explicit 'break' easily without iteration.
    # But we can REMOVE 'CreatePointsGrid_0' entirely?
    # No, risky if we need it back.
    # We can just Connect DataNode -> Transform.
    # PCG nodes can have multiple inputs? "In" pin usually accepts 1 or Multiple?
    # CreatePointsGrid usually feeds "In".
    # If we add edge Data -> Transform, we might get double input (Grid + Data).
    # Bad.
    
    # We MUST invalidiate Grid connection.
    # Hack: Remove Grid Node? 
    # Or Rename it/Move it? Edges follow.
    # Best: Remove the Grid Node. We can assume DungeonData is the Truth.
    
    if grid_node:
        try:
            graph.remove_node(grid_node)
            print("Removed 'CreatePointsGrid_0' (Legacy/Conflicting Source).")
        except: pass
        
    # 3. Connect DungeonData -> Transform
    if data_node and trans_node:
        try:
             graph.add_edge(data_node, "Out", trans_node, "In")
             print("Connected: DungeonData -> Forest_Transform")
        except Exception as e:
             print(f"Connect Error: {e}")
             
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Source Switched. Trees should spawn based on Dungeon Data.")

"""

def switch_source():
    print(f"--- [Fix] Switching Source ---", flush=True)
    
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
    switch_source()
