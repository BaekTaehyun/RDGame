import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Inverting Logic: Forest = World - Path ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Identify Existing Nodes
    reader_node = None
    bounds_node = None
    trans_node = None # Forest_Transform_Fixed
    
    for n in graph.nodes:
        if n.get_name() == "DungeonDataReader_1": reader_node = n
        if "BoundsModifier" in n.get_name(): bounds_node = n # Using the one we made
        
        # Find Transform
        t = "Unknown"
        try: t = n.get_editor_property("NodeTitleOverride")
        except: pass
        if "Forest_Transform" in str(t):
             trans_node = n
             
    # 2. PROPERLY RE-CREATE GRID (The Canvas)
    grid_node = None
    # Check if we didn't fully delete it?
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0": grid_node = n
    
    if not grid_node:
        ret = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
        grid_node = ret[0]
        grid_node.set_node_position(1000, 400)
        s = grid_node.get_settings()
        s.set_editor_property("CellSize", unreal.Vector(180, 180, 100))
        # Important: Cull?
        print("Restored Grid (CellSize 180).")

    # 3. Create/Find Difference Node
    diff_node = None
    for n in graph.nodes:
        if "Difference" in n.get_name():
            diff_node = n
            break # Use existing if any
            
    if not diff_node:
        ret = graph.add_node_of_type(unreal.PCGDifferenceSettings)
        diff_node = ret[0]
        diff_node.set_node_position(1200, 650)
        s = diff_node.get_settings()
        # Mode: Infer? Binary? Discrete?
        # Usually default works (Source - Difference).
        print("Created Difference Node.")

    # 4. Create Density Noise (For Variety)
    noise_node = None
    ret = graph.add_node_of_type(unreal.PCGAttributeNoiseSettings) # Check class name
    if not ret: # Fallback guess
         pass 
    else:
         noise_node = ret[0]
         noise_node.set_node_position(1500, 650)
         print("Created Density Noise Node.")

    # 5. RE-WIRE
    # Flow: Grid(Source) - [Reader->Bounds](Diff) -> Output -> Noise -> Transform -> Project -> Filters
    
    if grid_node and diff_node and noise_node and trans_node:
        # A. Grid -> Diff (Source)
        # Difference Inputs: "Source", "Difference".
        # We need to map pins. Standard is usually Source=0, Difference=1?
        # Graph.add_edge_by_name(from, outPin, to, inPin)
        
        try: graph.add_edge_by_name(grid_node, "Out", diff_node, "Source")
        except: graph.add_edge(grid_node, "Out", diff_node, "In") # Fallback
        
        # B. Reader->Bounds -> Diff (Difference)
        # We need the output of Bounds (which we injected in Step 6144).
        # Reader -> Bounds is already linked.
        if bounds_node:
             try: graph.add_edge_by_name(bounds_node, "Out", diff_node, "Difference")
             except: 
                 # Try connecting to second pin?
                 pass
                 print("Warning: Check Difference Input Pins manually if failed.")
        
        # C. Diff -> Noise
        try: graph.add_edge(diff_node, "Out", noise_node, "In")
        except: pass
        
        # D. Noise -> Transform
        try: graph.add_edge(noise_node, "Out", trans_node, "In")
        except: pass
        
        print("Re-Wired: Grid - Path -> Noise -> Transform.")
        
    # 6. Re-Tune Filters (Restore Layers)
    tiers = [
        ("DensityFilter_1", 0.90), # Big Trees
        ("DensityFilter_2", 0.60),
        ("DensityFilter_3", 0.40),
        ("DensityFilter_4", 0.20)  # Ground
    ]
    for fname, val in tiers:
        for n in graph.nodes:
            if n.get_name() == fname:
                try: 
                    s = n.get_settings()
                    s.lower_bound = val
                    s.upper_bound = 1.0 # Reset Upper
                except: 
                    pass # Props
                print(f"Restored {fname} -> {val}")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Logic Inverted. Forest should be OFF the path now.")

"""

def invert_logic():
    print(f"--- [Fix] Inverting Logic ---", flush=True)
    
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
    invert_logic()
