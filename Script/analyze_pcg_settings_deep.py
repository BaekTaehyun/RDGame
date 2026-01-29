import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] Analyzing Connections ---")

graph = unreal.load_asset(graph_path)
if graph:
    # Attempt to access Edges usually via property (not always exposed to Python)
    # If not exposed, we have to trust previous logic or check Pins.
    
    # Try getting edges
    try:
        # Some versions expose edges
        edges = graph.get_editor_property("Edges") # Might be 'GraphEdges'??
        # Actually usually it is not easy.
        pass
    except: pass
    
    # Let's inspect Node Inputs/Outputs pins if possible?
    # Or just print all nodes and their Upstream/Downstream if available.
    
    print(f"Nodes: {len(graph.nodes)}")
    
    # Check specific nodes settings
    for n in graph.nodes:
        name = n.get_name()
        if name in ["SelfPruning_0", "DensityFilter_5", "TransformPoints_2", "StaticMeshSpawner_5"]:
            print(f"[{name}]")
            # Settings
            s = n.get_settings()
            if s:
                 print(f"  Settings: {s.get_class().get_name()}")
                 # Dump key props
                 if "Density" in name:
                     try: print(f"  Range: {s.lower_bound} - {s.upper_bound}")
                     except: pass
                 if "Transform" in name:
                     try: print(f"  ScaleMin: {s.scale_min}, ScaleMax: {s.scale_max}") 
                     except: 
                        try: print(f"  ScaleMin: {s.get_editor_property('ScaleMin')}")
                        except: pass
                     try: print(f"  ApplyScale: {s.get_editor_property('ApplyScale')}")
                     except: pass
                 if "Spawner" in name:
                     selector = s.mesh_selector_parameters
                     try: 
                         entries = selector.get_editor_property("MeshEntries")
                         print(f"  MeshEntries Count: {len(entries)}")
                         for i, e in enumerate(entries):
                             desc = e.get_editor_property("Descriptor")
                             mesh = desc.get_editor_property("StaticMesh")
                             print(f"    [{i}] {mesh.get_name() if mesh else 'None'}")
                     except: print("  Cannot read descriptors")

"""

def analyze_connections():
    print(f"--- [Audit] Deep Dive Node Settings ---", flush=True)
    
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
    analyze_connections()
