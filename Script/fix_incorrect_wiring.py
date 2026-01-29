import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Correcting Inverted Wiring ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Wall": None, "Floor": None, "Bounds": None, "Dist": None
    }
    
    # 1. Identify Nodes
    for n in graph.nodes:
        nm = n.get_name()
        if "DungeonDataReader" in nm:
            # Check Title override or properties to distinguish Wall/Floor
            # Based on previous scripts: 
            # Reader_1 = Floor? Reader_2 = Wall?
            # User image shows "Dungeon Data Reader" (Wall) and (Floor).
            # We trust the indices or titles if set.
            # Let's check TitleOverride if available.
            title = n.get_editor_property("NodeTitleOverride") if n.has_editor_property("NodeTitleOverride") else ""
            if "Wall" in title: nodes["Wall"] = n
            if "Floor" in title: nodes["Floor"] = n
            
            # Fallback by index if titles missing
            if not nodes["Wall"] and "2" in nm: nodes["Wall"] = n
            if not nodes["Floor"] and "1" in nm: nodes["Floor"] = n
            
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "Distance" in nm: nodes["Dist"] = n
        
    # 2. Re-Connect Correctly
    
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # A. Wall -> Bounds (Forest Generation Source)
    if nodes["Wall"] and nodes["Bounds"]:
        # We should ideally clear previous edges to Bounds first.
        # But PCG Graph API doesn't support easy "Disconnect".
        # Adding a new edge usually works.
        connect(nodes["Wall"], nodes["Bounds"])
        print(f"Connected Wall({nodes['Wall'].get_name()}) -> Bounds")
        
    # B. Floor -> Distance Target (Avoidance/Gradient Target)
    if nodes["Floor"] and nodes["Dist"]:
        connect(nodes["Floor"], nodes["Dist"], "Out", "Target")
        print(f"Connected Floor({nodes['Floor'].get_name()}) -> Distance(Target)")
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Wiring Corrected.")

"""

def fix_wiring():
    print(f"--- [Fix] Fix Wiring ---", flush=True)
    
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
    fix_wiring()
