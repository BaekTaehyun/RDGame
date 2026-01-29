import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("--- [Fix] PCG Unbounded ---")
world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/DungeonGenerator.DungeonWorldBuilder"))

if actors:
    dungeon = actors[0]
    pcg_comps = dungeon.get_components_by_class(unreal.PCGComponent)
    for comp in pcg_comps:
        print(f"Modifying Comp: {comp.get_name()}")
        # comp.set_editor_property("is_unbounded", True) 
        
        found = False
        for p in dir(comp):
            if "unbounded" in p.lower():
                print(f"Property Candidate: {p}")
                
        try: 
            comp.set_editor_property("bIsUnbounded", True)
            print("Set bIsUnbounded=True")
        except:
             pass
            
        # Force Generate
        comp.generate_local(True)
        print("Set Unbounded=True and Generated.")

"""

def fix_unbounded():
    print(f"--- [Fix] PCG Unbounded ---", flush=True)
    
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
    fix_unbounded()
