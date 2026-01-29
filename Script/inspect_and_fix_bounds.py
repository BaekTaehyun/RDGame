import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Injecting Bounds Modifier (Invalid Bounds Fix) ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Nodes
    reader_node = None
    trans_node = None
    
    for n in graph.nodes:
        if n.get_name() == "DungeonDataReader_1": reader_node = n
        
        # Find Forest Transform (we know it's connected to Reader)
        # But we can look for "Forest_Transform_Fixed" title or similar
        t = "Unknown"
        try: t = n.get_editor_property("NodeTitleOverride")
        except: pass
        if "Forest_Transform" in str(t):
             trans_node = n
             
    # Fallback to find TransformPoints_1 if title fails
    if not trans_node:
        for n in graph.nodes:
            if n.get_name() == "TransformPoints_1": trans_node = n
            
    # 2. Inspect Reader
    if reader_node:
        try:
            s = reader_node.get_settings()
            # print query?
            pass
        except: pass

    # 3. Inject Bounds Modifier
    if reader_node and trans_node:
        # Create Modifier
        ret = graph.add_node_of_type(unreal.PCGBoundsModifierSettings)
        bounds_node = ret[0]
        bounds_node.set_node_position(1400, 650) # Between Reader(Left) and Transform(1600)
        
        s = bounds_node.get_settings()
        # Set Mode to 'Set' (1?) Usually Enum. 
        # Props: Mode, BoundsMin, BoundsMax.
        try:
             # Enum: Set=1 ? or 'Set' string? 
             # Let's try to set Min/Max first.
             b_size = 100.0
             s.set_editor_property("BoundsMin", unreal.Vector(-b_size, -b_size, -b_size))
             s.set_editor_property("BoundsMax", unreal.Vector(b_size, b_size, b_size))
             
             # Need to find enum value for 'Set'. Default is usually 'Scale' or 'Intersect'?
             # Let's assume default might accept vector addition.
             # Ideally we set it to 'Set'.
             # s.set_editor_property("Mode", unreal.PCGBoundsModifierMode.SET) <- If Enum exposed
        except Exception as e:
             print(f"Bounds Config Error: {e}")
             
        # 4. Re-Link: Reader -> Bounds -> Transform
        try:
            # Connect Reader -> Bounds
            graph.add_edge(reader_node, "Out", bounds_node, "In")
            
            # Connect Bounds -> Transform
            graph.add_edge(bounds_node, "Out", trans_node, "In")
            
            print("Connected: Reader -> BoundsModifier -> Transform")
        except Exception as e:
            print(f"Link Error: {e}")
            
    else:
        print("Missing Nodes (Reader or Transform).")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Bounds Modifier Injected.")

"""

def fix_bounds():
    print(f"--- [Fix] Fixing Bounds ---", flush=True)
    
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
    fix_bounds()
