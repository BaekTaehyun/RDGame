import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
print("--- [Debug] Force Connect V3 (Blind Force) ---")

graph = unreal.load_asset(graph_path)
if graph:
    nodes = {}
    for n in graph.nodes:
        nm = n.get_name()
        if "CreatePointsGrid" in nm: nodes["Grid"] = n
        if "CopyPoints" in nm: nodes["Copy"] = n
        if "DungeonDataReader" in nm:
            t = ""
            try: t = n.get_editor_property("NodeTitleOverride")
            except: pass
            if "Wall" in t or "2" in nm: nodes["Wall"] = n
            if "Floor" in t or "1" in nm: nodes["Floor"] = n
        if "TransformPoints" in nm:
            try:
                s = n.get_settings()
                off = s.get_editor_property("OffsetMin")
                if off.z > 500: nodes["Lift"] = n
                else: nodes["Trans"] = n
            except: pass
        if "Projection" in nm: nodes["Proj"] = n
        if "Distance" in nm: nodes["Dist"] = n
        if "DensityFilter" in nm and "F1" not in nodes: nodes["F1"] = n

    print(f"Nodes Found: {list(nodes.keys())}")

    pairs = [
        ("Grid", "Copy", "Out", "Source"),
        ("Wall", "Copy", "Out", "Target"),
        ("Copy", "Lift", "Out", "In"),
        ("Lift", "Proj", "Out", "In"),
        ("Proj", "Dist", "Out", "Source"),
        ("Floor", "Dist", "Out", "Target"),
        ("Dist", "Trans", "Out", "In"),
        ("Trans", "F1", "Out", "In")
    ]
    
    for src_key, dst_key, src_label, dst_label in pairs:
        src = nodes.get(src_key)
        dst = nodes.get(dst_key)
        if not src or not dst:
            print(f"SKIP {src_key}->{dst_key} (Missing Node)")
            continue
            
        print(f"Connecting {src_key} -> {dst_key}...", end="")
        try:
            # Using unreal.Name explicit cast
            res = graph.add_edge(src, unreal.Name(src_label), dst, unreal.Name(dst_label))
            print(f" Result: {res}")
        except Exception as e:
            print(f" Error: {e}")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Saved.")

    # Refresh
    try: unreal.DungeonAssetUtils.refresh_blueprint(graph)
    except: pass
"""

def force_connect_v3():
    print(f"--- [Debug] Force Connect V3 ---", flush=True)
    
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
    force_connect_v3()
