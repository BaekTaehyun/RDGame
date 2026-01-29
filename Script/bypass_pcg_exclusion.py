import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood" 
target_graph = unreal.load_asset(graph_path)

if not target_graph:
    print("Error: Could not load graph")
else:
    print(f"Modifying Graph: {target_graph.get_name()}")
    
    sampler_node = None
    transform_node = None
    
    for node in target_graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        
        class_name = settings.get_class().get_name()
        print(f"Node Class: {class_name}")

        if "PCGSurfaceSamplerSettings" in class_name:
            sampler_node = node
        elif "PCGTransformPointsSettings" in class_name:
            transform_node = node
            
    if sampler_node and transform_node:
        print(f"Connecting {sampler_node.node_title} -> {transform_node.node_title}")
        
        # Break existing inputs to Transform
        # We need to find the specific pins. Usually 'Out' to 'In'.
        
        # Use add_edge_to (SourceNode, SourcePinLabel, TargetNode, TargetPinLabel)
        # Note: Pin labels might be 'Out', 'In' or 'Output', 'Input' or 'Points'
        
        # Taking a guess based on standard nodes: 'Out' -> 'In'
        try:
            # Use 'Out' for Sampler and 'In' for Transform
            # Labels might need to be FName, but Python string usually converts.
            target_graph.add_edge(sampler_node, "Out", transform_node, "In")
            print("Edge Added: Sampler (Out) -> Transform (In)")
            
            unreal.EditorAssetLibrary.save_loaded_asset(target_graph)
            print("Graph Saved.")
        except Exception as e:
            print(f"Connection Failed: {e}")
            
    else:
        print(f"Nodes missing. Sampler={sampler_node}, Transform={transform_node}")
"""

def bypass_exclusion():
    print(f"--- [Fix] Bypass Exclusion ---", flush=True)
    
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
    bypass_exclusion()
