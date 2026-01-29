import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Audit] Inspecting Filter Inputs ---")

graph = unreal.load_asset(graph_path)
if graph:
    # Find DensityFilter_1
    target = None
    for n in graph.nodes:
        if n.get_name() == "DensityFilter_1":
            target = n
            break
            
    if target:
        # Check Upstream Edges?
        # Python API for checking incoming edges on a Node?
        # graph.get_all_upstream_nodes(target)? No.
        # We must iterate all edges in the graph. (This is expensive but graph is small)
        
        # Actually, Graph has 'get_edges()' ??
        # No. But we can iterate nodes and check "Out" pins?
        # Wait, how do we know connection count?
        # We can't easily without iterating everything.
        
        # Let's count how many nodes point TO DensityFilter_1.
        incoming_count = 0
        sources = []
        
        # We need to access the PCGGraph's edge list.
        # In Python, we usually can't iterate edges directly unless exposed.
        # 'graph.get_editor_property("Edges")' might work?
        try:
             # Edges is a property.
             pass 
        except: pass
        
        # Alternative: We know we connected "Forest_Transform".
        # We suspect "CopyPoints_0" (or Grid) is also connected.
        
        print("Inspection via Node iteration (Simulated):")
        # Since we can't iterate Edges, we will assume Double Connection exists 
        # based on my previous script doing 'add_edge' without removing old.
        
        print("Assumption: Double Connection Exists.")
        
        # Action: Try to Disconnect "CopyPoints_0" -> "DensityFilter_1".
        # Or better: Disconnect EVERYTHING from DensityFilter_1, then Reconnect ONLY Transform.
        # graph.break_all_node_links(target)  <-- Breaks Inputs AND Outputs. Bad.
        
        # graph.break_pin_links(node, pin) ?
        # "In" pin.
        
        # Let's try to find if 'break_pin_links' exists.
        found_break = False
        if hasattr(graph, "break_pin_links"): found_break = True
        
        print(f"Has break_pin_links: {found_break}")
        
    else:
        print("Target Filter Not Found.")

"""

def inspect_inputs():
    print(f"--- [Audit] Input Check ---", flush=True)
    
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
