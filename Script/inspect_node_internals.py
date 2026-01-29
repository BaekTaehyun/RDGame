import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Inspect] Node Internals ---")

graph = unreal.load_asset(graph_path)
if graph:
    # Pick one node: SurfaceSampler
    target = None
    for n in graph.nodes:
        if "SurfaceSampler" in n.get_name():
            target = n
            break
            
    if target:
        print(f"Node: {target.get_name()} ({target.get_class().get_name()})")
        
        # 1. Print Dir
        print("Dir(node): ", [x for x in dir(target) if not x.startswith("_")])
        
        # 2. Try to get Pins via 'call_method' or properties
        # In newer PCG, pins might be exposed differently.
        # Try 'get_input_pins', 'input_pins'
        
        try:
            pins = target.get_input_pins()
            print(f"GetInputPins(): {len(pins)}")
            for p in pins:
                print(f"  Pin: {p.label} ({p.properties.label})")
        except Exception as e:
            print(f"GetInputPins failed: {e}")
            
        # 3. Settings Inspection
        settings = target.get_settings()
        if settings:
             print(f"Settings: {settings.get_class().get_name()}")
             # Some settings have 'OutputPinProperties'
             try:
                 props = settings.get_editor_property("OutputPinProperties")
                 print(f"OutputPinProperties: {len(props)}")
                 for p in props:
                     print(f"  Label: {p.label}")
             except: pass
             
"""

def inspect_internals():
    print(f"--- [Inspect] Internals ---", flush=True)
    
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
    inspect_internals()
