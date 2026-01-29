import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Inspect] Landscape Tags ---")
world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)

print(f"Found {len(actors)} Landscape(s).")
for a in actors:
    print(f"Name: {a.get_name()}")
    print(f"Label: {a.get_actor_label()}")
    print(f"Tags: {a.tags}")
    print(f"Components: {len(a.get_components_by_class(unreal.PCGComponent))}")
    print("-" * 20)
"""

def check_landscape_tag():
    print(f"--- [Inspect] Landscape Tag ---", flush=True)
    
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
    check_landscape_tag()
