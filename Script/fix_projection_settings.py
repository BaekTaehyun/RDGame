import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Fix] Projection Settings ---")

graph = unreal.load_asset(graph_path)
if graph:
    proj_node = None
    for n in graph.nodes:
        if "Projection" in n.get_name():
            proj_node = n
            break
            
    if proj_node:
        settings = proj_node.get_settings()
        
        # Inspection
        try:
            print(f"Current Target: {settings.get_editor_property('ProjectionTarget')}")
        except: print("Could not read ProjectionTarget")
        
        try:
            print(f"Current Source: {settings.get_editor_property('ProjectionSource')}") # For 'Project' mode usually Source=Point, Target=Surface
        except: pass

        # Force Settings
        try:
            # ProjectionTarget Enum:
            # 0 = Blueprint/Actor?
            # 1 = Landscape? 
            # 2 = World?
            # Standard PCG projection usually allows "Landscape". 
            # Let's try attempting to set it to 'Landscape' if it accepts string (unlikely) or iterate via Enum.
            # Safe bet: "World" (hits everything static).
            # But let's try to find property names.
            pass
        except: pass
        
        # Hard Fix attempt: Set to 'Landscape' (Often Index 1)
        # Or 'LandscapeHeight'
        # Let's try to set "Mode" to "Project" (Method).
        
        # Property: "ProjectionMethod" -> Project
        # Property: "ProjectionParams.ProjectionTarget" ?
        
        # Actually PCGProjectionSettings has:
        # - Target (Enum)
        # - Attribute params...
        
        # Let's simple-set 'Target' to 1 (Landscape) or 2 (Other).
        # We'll set 'bProjectPositions' = True.
        
        try:
            # Try setting enum by string?
            settings.set_editor_property("ProjectionTarget", unreal.PCGProjectionTarget.LANDSCAPE)
            print("Set ProjectionTarget -> LANDSCAPE")
        except:
            try:
                # Fallback indices
                settings.set_editor_property("ProjectionTarget", 1) 
                print("Set ProjectionTarget -> 1 (Index)")
            except Exception as e:
                print(f"ProjectionTarget Error: {e}")

        unreal.EditorAssetLibrary.save_loaded_asset(graph)
        print("Projection Updated.")
    else:
        print("Projection Node Not Found.")

"""

def fix_proj():
    print(f"--- [Fix] Projection Config ---", flush=True)
    
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
    fix_proj()
