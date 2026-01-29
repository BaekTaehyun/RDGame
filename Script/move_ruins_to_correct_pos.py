import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

print("--- [Fix] Moving Ruins Nodes ---")

graph = unreal.load_asset(graph_path)
if not graph:
    print("Error: Graph not found!")
else:
    # Target Names (from previous step)
    targets = {
        "DensityFilter_5": (1000, 1200),
        "TransformPoints_2": (1300, 1200),
        "StaticMeshSpawner_5": (1600, 1200)
    }
    
    count = 0
    for n in graph.nodes:
        name = n.get_name()
        if name in targets:
            tx, ty = targets[name]
            print(f"Moving {name} to ({tx}, {ty})...")
            
            # Try multiple ways to set position
            done = False
            
            # 1. Standard Attribute
            try:
                n.position_x = tx
                n.position_y = ty
                done = True
            except: pass
            
            # 2. Editor Property (PositionX)
            if not done:
                try:
                    n.set_editor_property("PositionX", tx)
                    n.set_editor_property("PositionY", ty)
                    done = True
                except: pass
                
            # 3. Editor Property (NodePosX)
            if not done:
                try:
                    n.set_editor_property("NodePosX", tx)
                    n.set_editor_property("NodePosY", ty)
                    done = True
                except: pass
            
            # 4. Method Call
            if not done:
                try:
                    n.set_node_position(tx, ty)
                    done = True
                except Exception as e:
                     print(f"set_node_position failed: {e}")
            
            if done:
                count += 1
            else:
                print(f"Failed to set position for {name}")
                # Inspect properties if failed
                print(f"Failed to set position for {name}. Dumping props:")
                for d in dir(n):
                    if "pos" in d.lower():
                        val = getattr(n, d)
                        print(f"  {d}: {val}")
                # Try to see if it is 'NodePosition' struct
                # or 'Position'
                break # Just inspect one to save log space

    if count > 0:
        unreal.EditorAssetLibrary.save_loaded_asset(graph)
        print(f"Moved {count} nodes and Saved.")
    else:
        print("No nodes found or moved.")
"""

def move_nodes():
    print(f"--- [Fix] Relocating Nodes ---", flush=True)
    
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
    move_nodes()
