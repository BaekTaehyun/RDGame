import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Inspect] Hybrid Pins ---")

graph = unreal.load_asset(graph_path)
if graph:
    # Helper to print pins
    def print_pins(node, label):
        print(f"\\n[{label}] {node.get_name()}")
        
        # In PCG, pins are often dynamic or hard to reach via Python API directly on the node object 
        # unless we use specific getters.
        # But 'PCGNode' has 'input_pins' and 'output_pins' properties in newer versions.
        # Let's try to access via property reflection first.
        
        try:
            ins = node.get_editor_property("InputPins")
            print(f"  Inputs ({len(ins)}):")
            for p in ins:
                l = p.get_editor_property("Label")
                # visible name might not be internal name
                print(f"    - Label: '{l}'")
        except:
             print("  (Cannot read InputPins directly)")

        try:
            outs = node.get_editor_property("OutputPins")
            print(f"  Outputs ({len(outs)}):")
            for p in outs:
                l = p.get_editor_property("Label")
                print(f"    - Label: '{l}'")
        except:
             print("  (Cannot read OutputPins directly)")

    for n in graph.nodes:
        nm = n.get_name()
        
        # Check specific types
        if "GetLandscape" in nm: print_pins(n, "Landscape")
        if "SurfaceSampler" in nm: print_pins(n, "Sampler")
        if "Union" in nm: print_pins(n, "Union")
        if "BoundsModifier" in nm: print_pins(n, "Bounds")
        if "Difference" in nm: print_pins(n, "Difference")
        if "TransformPoints" in nm: print_pins(n, "Transform")
            
"""

def inspect_pins():
    print(f"--- [Inspect] Pins ---", flush=True)
    
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
    inspect_pins()
