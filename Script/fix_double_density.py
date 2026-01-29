import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Removing Double Density & Re-Tuning ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. Find Critical Nodes
    grid_node = None
    copy_node = None
    trans_node = None
    
    # CreatePointsGrid_0
    for n in graph.nodes:
        if n.get_name() == "CreatePointsGrid_0": grid_node = n
        if n.get_name() == "CopyPoints_0": copy_node = n
        
    # Find Transform by Connection or Class
    # We know we connected Grid -> Transform
    # But inspecting edges is hard.
    # Let's iterate all TransformNodes.
    
    candidates = []
    for n in graph.nodes:
        s = n.get_settings()
        if s and "PCGTransformPointsSettings" in s.get_class().get_name():
            name = n.get_name()
            # Ruins is TransformPoints_2 (or connected to Ruins Filter)
            # We want the one that is NOT connected to Ruins.
            # Or just the one that ISN'T TransformPoints_2?
            # Let's assume the NEWest one or check Title again differently.
            candidates.append(n)
            
    if candidates:
         # Pick the one that matches our logic (Created recently)
         # Identify by title property raw access?
         for c in candidates:
             try: 
                 t = c.get_editor_property("NodeTitleOverride")
                 if "Forest" in str(t):
                     trans_node = c
                     break
             except: pass
         
         # If still not found, pick the one that IS NOT TransformPoints_2 (Ruins)
         if not trans_node and len(candidates) > 0:
              for c in candidates:
                  if c.get_name() != "TransformPoints_2":
                      trans_node = c
                      print(f"Guessed Forest Transform: {c.get_name()}")
                      break

    if not trans_node:
        print("Critical: Forest_Transform NOT found (Even by Class).")
    else:
        # 2. Re-Connect Transform to ALL downstream candidates (Filters)
        # Just to be safe before we cut the line.
        # We target DensityFilters (1-5? No, 5 is Ruins). 1-4.
        # And AttributeFilters (1-4).
        
        targets = []
        for i in range(1, 5):
            targets.append(f"DensityFilter_{i}")
            targets.append(f"AttributeFilter_{i}")
            
        print("Ensuring Transform Connections...")
        for tname in targets:
            for n in graph.nodes:
                if n.get_name() == tname:
                     try: graph.add_edge(trans_node, "Out", n, "In")
                     except: pass

        # 3. Remove CopyPoints_0 (The source of Double Density)
        if copy_node:
            try:
                graph.remove_node(copy_node)
                print("Removed CopyPoints_0 (Old Path Broken).")
            except Exception as e:
                print(f"Remove Error: {e}")
        else:
            print("CopyPoints_0 already gone.")

    # 4. Tune Big Trees (Layer 0) - User compliant: "Still too dense"
    # Set to 0.90 (Top 10%)
    for n in graph.nodes:
        if n.get_name() == "DensityFilter_1":
            try:
                s = n.get_settings()
                try: s.lower_bound = 0.90
                except: s.set_editor_property("LowerBound", 0.90)
                print("DensityFilter_1 (Big Trees) -> 0.90")
            except: pass

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Fixed.")

"""

def fix_double():
    print(f"--- [Fix] Fixing Double Density ---", flush=True)
    
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
    fix_double()
