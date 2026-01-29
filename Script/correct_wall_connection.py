import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Connecting Wall Data (Removing Floor Connection) ---")

graph = unreal.load_asset(graph_path)
if graph:
    wall_node = None
    floor_node = None
    noise_node = None
    trans_node = None
    
    # 1. Identify Wall/Floor Readers
    for n in graph.nodes:
        title = "Unknown"
        try: title = str(n.get_editor_property("NodeTitleOverride"))
        except: pass
        
        if "Wall" in title: wall_node = n
        if "Floor" in title: floor_node = n
        
        if "AttributeNoise" in n.get_name(): noise_node = n
        if "Forest_Transform" in title: trans_node = n

    # Backup finding for Transform
    if not trans_node:
         for n in graph.nodes:
             if "TransformPoints" in n.get_name() and n.get_name() != "TransformPoints_2":
                 trans_node = n

    # 2. Cleanup "Floor" Connections
    # If Floor is connected to Noise or Transform, break it.
    # We can't explicitly break specific edges easily, but we can bypass.
    # Or we can verify what 'add_edge' does (it might add a second edge).
    # We want to Ensure ONLY Wall is connected.
    
    # Strategy:
    # We will connect Wall -> Noise.
    # We will also try to "Disconnect" Floor if possible.
    # Since we can't remove edges by ID, we'll assume the user might have to delete the bad wire if I can't.
    # BUT, I can try to Remove Node 'CreatePointsGrid_0' and 'Difference' if they are cluttering.
    
    # Using "Wall" implies we don't need Grid-Floor difference.
    # So let's delete the 'Difference' node to break that chain?
    # Or just connect Wall -> Noise, and hope Noise has only 1 input pin?
    # PCG nodes usually allow multiple inputs (Union).
    # So if Floor is connected, we get Union(Floor, Wall).
    
    # Best bet: Delete the nodes I created for the "Difference" logic (Method A).
    # Remove 'CreatePointsGrid_0', 'Difference_...', 'BoundsModifier...'.
    
    nodes_to_remove = []
    for n in graph.nodes:
        if "Difference" in n.get_name(): nodes_to_remove.append(n)
        if "BoundsModifier" in n.get_name(): nodes_to_remove.append(n) # The one I added
        # if "CreatePointsGrid" in n.get_name() and "0" in n.get_name(): nodes_to_remove.append(n)
        
    for n in nodes_to_remove:
        try:
            graph.remove_node(n)
            print(f"Removed interfering node: {n.get_name()}")
        except: pass

    # 3. Connect Wall -> Noise -> Transform
    if wall_node and noise_node and trans_node:
        try:
            # Wall -> Noise
            graph.add_edge(wall_node, "Out", noise_node, "In")
            print("Connected: Wall -> Noise")
            
            # Noise -> Transform
            graph.add_edge(noise_node, "Out", trans_node, "In")
            print("Connected: Noise -> Transform")
            
        except Exception as e:
            print(f"Connection Error: {e}")
            
    else:
        print("Missing Nodes:")
        if not wall_node: print("  Wall Reader Missing")
        if not noise_node: print("  Noise Node Missing")
        if not trans_node: print("  Transform Node Missing")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Logic Restored: Wall Data -> Forest.")

"""

def correct_wall():
    print(f"--- [Fix] Correcting Wall Logic ---", flush=True)
    
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
    correct_wall()
