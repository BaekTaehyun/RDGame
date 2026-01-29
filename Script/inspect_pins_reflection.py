import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Inspect] Pins via Reflection ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = []
    
    # helper
    def inspect(node):
        print(f"\\nNode: {node.get_name()} ({node.get_class().get_name()})")
        
        # Try InputPins Property
        try:
            # Note: For PCGNode, pins might be in a property named 'InputPins' or similar.
            # It's an array of FPCGPin.
            in_pins = node.get_editor_property("InputPins")
            print(f"  Input Pins ({len(in_pins)}):")
            for p in in_pins:
                # FPCGPin has 'Properties' -> FPCGPinProperties -> Label (FName)
                try:
                    props = p.get_editor_property("Properties")
                    label = props.get_editor_property("Label")
                    print(f"    - '{label}'")
                except:
                    print(f"    - (Cannot read Label)")
        except Exception as e:
            print(f"  [Error Reading Inputs] {e}")

        # Try OutputPins Property
        try:
            out_pins = node.get_editor_property("OutputPins")
            print(f"  Output Pins ({len(out_pins)}):")
            for p in out_pins:
                try:
                    props = p.get_editor_property("Properties")
                    label = props.get_editor_property("Label")
                    print(f"    - '{label}'")
                except:
                    print(f"    - (Cannot read Label)")
        except Exception as e:
            print(f"  [Error Reading Outputs] {e}")

    # Check key nodes
    for n in graph.nodes:
        nm = n.get_name()
        if "CopyPoints" in nm or "CreatePointsGrid" in nm or "DungeonDataReader" in nm:
            inspect(n)

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
