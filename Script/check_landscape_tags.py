import sys
import json
import subprocess

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== Check Landscape Tags ===")

world = unreal.EditorLevelLibrary.get_editor_world()

# Find ALL Landscapes
landscapes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)
for ls in landscapes:
    tags = [str(t) for t in ls.tags]
    print(f"Landscape: {ls.get_actor_label()}")
    print(f"  - Tags: {tags}")
    print(f"  - Has DungeonGeneratedLandscape tag: {'DungeonGeneratedLandscape' in tags}")

# Find by Tag
tagged = []
unreal.GameplayStatics.get_all_actors_with_tag(world, "DungeonGeneratedLandscape", tagged)
print(f"\\nActors with 'DungeonGeneratedLandscape' tag: {len(tagged)}")
for a in tagged:
    print(f"  - {a.get_actor_label()} ({a.get_class().get_name()})")

print("\\n=== Done ===")
"""

def check_landscape_tags():
    print(f"--- [Check Landscape Tags] ---", flush=True)
    
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
    check_landscape_tags()
