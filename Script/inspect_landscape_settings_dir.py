import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood" 
target_graph = unreal.load_asset(graph_path)

if target_graph:
    for node in target_graph.nodes:
        settings = node.get_settings()
        if settings and "PCGGetLandscapeSettings" in settings.get_class().get_name():
            print(f"--- Properties of {settings.get_class().get_name()} ---")
            for p in dir(settings):
                if not p.startswith("_"):
                    print(p)
            
            # Inspect ActorSelector property if found
            if hasattr(settings, "actor_selector"):
                 print("--- ActorSelector Details ---")
                 sel = settings.actor_selector
                 for p in dir(sel):
                    if not p.startswith("_"):
                        print(f"{p}: {getattr(sel, p)}")
"""

def inspect_params():
    print(f"--- [Inspect] PCG Dir ---", flush=True)
    
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
    inspect_params()
