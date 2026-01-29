import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Inspect] Internal Edge Verification ---")

graph = unreal.load_asset(graph_path)
if graph:
    # Helper to print connections
    def check_outputs(node, label_prefix=""):
        name = node.get_name()
        try:
            # We access OutputPins via reflection or get_output_pins() if available?
            # Previous error said 'get_output_pins' missing on PCGNode.
            # So use reflection on 'OutputPins' property.
            out_pins = node.get_editor_property("OutputPins")
            
            print(f"{label_prefix}[Node: {name}] has {len(out_pins)} Output Pins.")
            
            for p in out_pins:
                p_label = p.get_editor_property("Properties").get_editor_property("Label")
                
                # Edges are in 'Edges' array of FPCGPin
                edges = p.get_editor_property("Edges") # Array of UPCGEdge
                
                if len(edges) == 0:
                    print(f"{label_prefix}  - Pin '{p_label}': NO EDGES")
                else:
                    print(f"{label_prefix}  - Pin '{p_label}': {len(edges)} Connection(s):")
                    for e in edges:
                        try:
                            # UPCGEdge has 'InputPin' (Upstream) and 'OutputPin' (Downstream)
                            # Wait, naming is confusing in PCG.
                            # Usually InputPin is the Pin on the Input Node? No.
                            # Edge connects A.Out -> B.In.
                            # In PCGEdge: 'InputPin' is the pin on the Output side of Source Node?
                            # 'OutputPin' is the pin on the Input side of Target Node?
                            # Let's inspect the Node attached to the 'OutputPin' (Downstream).
                            
                            down_pin = e.get_editor_property("OutputPin") # The pin on the destination node
                            down_node = down_pin.get_editor_property("Node")
                            down_label = down_pin.get_editor_property("Properties").get_editor_property("Label")
                            
                            print(f"{label_prefix}    -> {down_node.get_name()} [{down_label}]")
                        except Exception as ex:
                            print(f"{label_prefix}    -> (Error reading edge: {ex})")
                            
        except Exception as e:
            print(f"{label_prefix}  [Error] {e}")

    # Check Key Nodes
    for n in graph.nodes:
        nm = n.get_name()
        if "CreatePointsGrid" in nm: check_outputs(n, "  ")
        if "DungeonDataReader" in nm: check_outputs(n, "  ")
        if "CopyPoints" in nm: check_outputs(n, "  ") # Check if Copy connects to Lift
"""

def verify_edges():
    print(f"--- [Debug] Verify Edges ---", flush=True)
    
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
    verify_edges()
