import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Complete PCG Graph Connection Dump ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # Build node lookup
    node_names = {}
    for node in graph.nodes:
        settings = node.get_settings()
        if settings:
            class_name = settings.get_class().get_name().replace("Settings", "")
            # Count duplicates
            count = sum(1 for n in node_names.values() if class_name in n)
            name = f"{class_name}_{count}" if count > 0 else class_name
            node_names[node] = name
    
    print("\\n=== All Nodes ===")
    for node, name in node_names.items():
        print(f"  {name}")
    
    # Try to get edges via inspection
    print("\\n=== Trying to find edges ===")
    
    # Check if graph has get_edges or similar
    for attr in dir(graph):
        if 'edge' in attr.lower() or 'connect' in attr.lower() or 'link' in attr.lower():
            print(f"  Found: {attr}")
    
    # Check node for input/output pins
    print("\\n=== Node Pin Analysis ===")
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        class_name = settings.get_class().get_name().replace("Settings", "")
        
        # Check for input/output pins
        print(f"\\n[{class_name}]")
        
        # Try get_input_pins / get_output_pins
        try:
            in_pins = node.get_input_pins()
            print(f"  Input Pins: {len(in_pins)}")
            for pin in in_pins:
                # Check connected
                label = str(pin.properties.label) if hasattr(pin, 'properties') else str(pin)
                print(f"    - {label}")
        except Exception as e:
            print(f"  Input Pins Error: {e}")
        
        try:
            out_pins = node.get_output_pins()
            print(f"  Output Pins: {len(out_pins)}")
            for pin in out_pins:
                label = str(pin.properties.label) if hasattr(pin, 'properties') else str(pin)
                print(f"    - {label}")
        except Exception as e:
            print(f"  Output Pins Error: {e}")

    # Try to find edges through graph's edge list
    print("\\n=== Checking Graph Properties ===")
    for prop in ['edges', 'connections', 'links']:
        try:
            val = getattr(graph, prop)
            print(f"{prop}: {val}")
        except:
            pass

print("\\n=== Done ===")
"""

def dump_connections():
    print(f"--- [Dump PCG Connections] ---", flush=True)
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
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
    dump_connections()
