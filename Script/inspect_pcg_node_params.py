import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood_v2" 
# Note: User might be using v2 or original. Checking both or checking AssetRegistry.
asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
assets = asset_reg.get_assets_by_path("/Game/LevelPrototyping")

target_graph = None
for a in assets:
    if a.asset_name == "PCG_Nature_Wood":
        target_graph = unreal.load_asset(a.package_name)
        break

if not target_graph:
    print("Error: Could not find PCG_Nature_Wood")
else:
    print(f"Inspecting Graph: {target_graph.get_name()}")
    
    # Iterate nodes
    for node in target_graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        
        start_name = settings.get_class().get_name()
        
        if "PCGGetLandscapeSettings" in start_name:
            print(f"--- Found Landscape Node: {node.node_title} ---")
            # Reflection to get properties
            import json
            
            # Check Actor Selection
            # Note: Property names might be 'ActorSelection', 'ActorSelector', etc.
            # We will try to dump properties.
            
            def get_prop(obj, name):
                try: return obj.get_editor_property(name)
                except: return "N/A"

            print(f"ActorSelection: {get_prop(settings, 'actor_selection')}") 
            # It usually is a Struct 'PCGActorSelectorSettings' inside.
            
            # Let's look for specific tag logic
            # Sometimes it's directly on settings or inside a struct
            print(f"Properties: {settings.as_string()}") 
"""

def inspect_node():
    print(f"--- [Inspect] PCG Node Params ---", flush=True)
    
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
    inspect_node()
