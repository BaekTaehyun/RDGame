import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Node Input/Output Pin Analysis ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        cname = settings.get_class().get_name().replace("Settings", "")
        
        print(f"\\n[{cname}]")
        
        # Check input pins
        try:
            in_pins = node.get_input_pins()
            print(f"  Inputs: {len(in_pins)}")
            for pin in in_pins:
                try:
                    label = pin.properties.label
                    is_connected = pin.is_connected()
                    num_edges = pin.edge_count()
                    print(f"    {label}: connected={is_connected}, edges={num_edges}")
                except Exception as e:
                    print(f"    Pin info error: {e}")
        except Exception as e:
            print(f"  Input pin error: {e}")
        
        # Check output pins
        try:
            out_pins = node.get_output_pins()
            print(f"  Outputs: {len(out_pins)}")
            for pin in out_pins:
                try:
                    label = pin.properties.label
                    is_connected = pin.is_connected()
                    num_edges = pin.edge_count()
                    print(f"    {label}: connected={is_connected}, edges={num_edges}")
                except Exception as e:
                    print(f"    Pin info error: {e}")
        except Exception as e:
            print(f"  Output pin error: {e}")

print("\\n=== Done ===")
"""

def analyze_pins():
    print(f"--- [PCG Pin Analysis] ---", flush=True)
    
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
    analyze_pins()
