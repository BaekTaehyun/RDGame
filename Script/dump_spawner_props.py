import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
OUTPUT_FILE = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Temp_Spawner_Props.txt"

PYTHON_CODE = f"""
import unreal
import os

spawner_node_name = "StaticMeshSpawner_4"

graph = unreal.load_asset("/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood")
found = False
if graph:
    for n in graph.nodes:
        if n.get_name() == spawner_node_name:
            settings = n.get_settings()
            if settings:
                with open(r"{OUTPUT_FILE}", "w", encoding="utf-8") as f:
                    f.write(f"--- Properties of {{settings.get_class().get_name()}} ---\\n")
                    for a in dir(settings):
                        val_str = "<Error>"
                        try:
                            val = getattr(settings, a)
                            val_str = str(val)
                        except: pass
                        f.write(f"{{a}}: {{val_str}}\\n")
                found = True
            break
if not found:
    print("Node not found")
else:
    print("Dump complete")
"""

def dump_props():
    print(f"--- [Debug] Dumping All Spawner Props ---", flush=True)
    
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
    dump_props()
