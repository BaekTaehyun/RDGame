import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Correcting Inverted Wiring (V2) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {
        "Wall": None, "Floor": None, "Bounds": None, "Dist": None
    }
    
    # 1. Identify Nodes (Robust)
    for n in graph.nodes:
        nm = n.get_name()
        if "DungeonDataReader" in nm:
            title = ""
            try: title = n.get_editor_property("NodeTitleOverride")
            except: pass
            
            if "Wall" in title: nodes["Wall"] = n
            elif "Floor" in title: nodes["Floor"] = n
            else:
                # Fallback Indices
                if "2" in nm: nodes["Wall"] = n  # Usually 2 is Wall
                if "1" in nm: nodes["Floor"] = n # Usually 1 is Floor
            
        if "BoundsModifier" in nm: nodes["Bounds"] = n
        if "Distance" in nm: nodes["Dist"] = n
        
    # 2. Re-Connect Correctly
    
    def connect(src, dst, sp="Out", dp="In"):
        try: graph.add_edge(src, sp, dst, dp)
        except: pass

    # A. Wall -> Bounds (Forest Generation Source)
    if nodes["Wall"] and nodes["Bounds"]:
        connect(nodes["Wall"], nodes["Bounds"])
        print(f"Connected Wall({nodes['Wall'].get_name()}) -> Bounds")
        
    # B. Floor -> Distance Target (Avoidance/Gradient Target)
    if nodes["Floor"] and nodes["Dist"]:
        connect(nodes["Floor"], nodes["Dist"], "Out", "Target")
        print(f"Connected Floor({nodes['Floor'].get_name()}) -> Distance(Target)")
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Wiring V2 Corrected.")

# Force Gen
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target_actor = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == "PCGNature":
        target_actor = a
        break
if target_actor:
    pcg_comp = target_actor.get_component_by_class(unreal.PCGComponent)
    if pcg_comp:
        try: pcg_comp.generate(True)
        except: pass

"""

def fix_wiring_v2():
    print(f"--- [Fix] Fix Wiring V2 ---", flush=True)
    
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
    fix_wiring_v2()
