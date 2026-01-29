import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood" 
target_graph = unreal.load_asset(graph_path)

if not target_graph:
    print("Error: Could not load graph")
else:
    print(f"Modifying Graph: {target_graph.get_name()}")
    
    modified = False
    for node in target_graph.nodes:
        settings = node.get_settings()
        if settings and "PCGGetLandscapeSettings" in settings.get_class().get_name():
            print(f"--- Modifying Landscape Node: {node.node_title} ---")
            
            # Access ActorSelector
            sel = settings.actor_selector
            
            # Change to By Class (Generic) or By Tag
            # Let's try enforcing Tag search for robustness
            sel.set_editor_property("actor_selection", unreal.PCGActorSelection.BY_TAG)
            sel.set_editor_property("actor_selection_tag", "DungeonGeneratedLandscape")
            sel.set_editor_property("must_overlap_self", False) # Disable overlap check to be safe
            
            # Update settings
            settings.set_editor_property("actor_selector", sel)
            
            print("Updated ActorSelector to search for Tag 'DungeonGeneratedLandscape'")
            modified = True
            
    if modified:
        unreal.EditorAssetLibrary.save_loaded_asset(target_graph)
        print("Graph Saved.")
    else:
        print("No Landscape Node found.")
"""

def fix_node():
    print(f"--- [Fix] PCG Landscape Tag ---", flush=True)
    
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
    fix_node()
