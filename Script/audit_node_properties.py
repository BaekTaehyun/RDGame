import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] PCG Settings ---")

graph = unreal.load_asset(graph_path)
if graph:
    def print_props(obj, label):
        print(f"\\n[{label}] Class: {obj.get_class().get_name()}")
        # List interesting props
        props = [
            "BoundsMin", "BoundsMax", "Mode", # BoundsModifier
            "GridExtents", "CoordinateSpace", "CellSize", # Grid
            "ProjectionTarget", # Projection
            "SourceMode", "TargetMode" # Copy
        ]
        
        for p in props:
            try:
                if obj.has_editor_property(p):
                    val = obj.get_editor_property(p)
                    print(f"  {p}: {val}")
            except Exception as e:
                print(f"  {p}: (Error {e})")

    for n in graph.nodes:
        nm = n.get_name()
        
        # We need to access the SETTINGS object usually
        # PCGNode -> GetSettings() (wrapper) -> UPCGSettings
        try:
            settings = n.get_settings()
            if not settings: continue
            
            if "BoundsModifier" in nm:
                print_props(settings, f"Node: {nm}")
                
            if "CreatePointsGrid" in nm:
                print_props(settings, f"Node: {nm}")
                
            if "Projection" in nm:
                 print_props(settings, f"Node: {nm}")
                 
        except: pass
"""

def audit_props():
    print(f"--- [Audit] Props ---", flush=True)
    
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
    audit_props()
