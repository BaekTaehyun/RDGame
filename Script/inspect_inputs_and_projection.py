import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] Inputs & Projection ---")

graph = unreal.load_asset(graph_path)
if graph:
    # 1. List All Nodes to find Sources
    print("--- Nodes ---")
    sources = []
    proj_node = None
    
    for n in graph.nodes:
        name = n.get_name()
        cname = n.get_settings().get_class().get_name() if n.get_settings() else "None"
        
        # Identify Sources
        if "Grid" in name or "Input" in name or "Actor" in name or "Data" in name:
            sources.append((name, cname))
            
        # Identify Projection
        if "Projection" in name:
            proj_node = n
            
    for name, cname in sources:
        print(f"Source Candidate: {name} ({cname})")

    # 2. Inspect Projection Settings
    if proj_node:
        print(f"--- Projection Settings ({proj_node.get_name()}) ---")
        s = proj_node.get_settings()
        if s:
            # Print Props
            # ProjectionTarget (Enum), KeepZeroDensity, etc.
            # We can try to read 'ProjectionTarget'.
            try:
                # Iterate all props to see what's set
                 # Or just print the string repr of settings
                 # But usually we want specific values.
                 # Let's try key ones.
                 pass
            except: pass
            
            # Since we can't iterate easily, let's just confirm it exists.
            print("Projection Node Exists.")
    else:
        print("MISSING: Projection Node.")

    # 3. Check what feeds 'DensityFilter_1'
    # We can't iterate inputs easily, but we know what we connected.
    # We connected 'Projection' to 'Filter_1'.
    
"""

def inspect_inputs():
    print(f"--- [Audit] Inspecting ---", flush=True)
    
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
    inspect_inputs()
