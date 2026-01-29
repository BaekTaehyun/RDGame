import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Volume -> Output Debug ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    
    # 1. Find Volume Sampler (Created in previous step)
    source_node = None
    output_node = None
    
    for node in graph.nodes:
        # Check node Title or Class
        if node.node_title == "DEBUG_VOLUME" or "CreatePointsGrid" in node.get_settings().get_class().get_name():
            source_node = node
            
        # Check Output Node
        # Output node usually has no settings or specific class?
        # In PCG, Input/Output are usually graph parameters.
        # But there is a node called 'Output' or 'GraphOutput'?
        # Let's search for it.
        if "GraphOutput" in node.get_settings().get_class().get_name() or "Info" in node.get_class().get_name():
             # Actually, PCG graph structure differs.
             # Use get_output_node() API if available (I saw it in dir()!)
             pass

    try:
        output_node = graph.get_output_node()
        print(f"Output Node Found: {output_node}")
    except:
        print("get_output_node() failed, searching manually...")
        # Search manually
        pass
        
    if source_node and output_node:
        print(f"Connecting {source_node.get_settings().get_class().get_name()} -> Output")
        
        try:
            # Connect Source "Out" -> Output "In"
            graph.add_edge(source_node, "Out", output_node, "In")
            print("Connected to Output!")
        except Exception as e:
            print(f"Connection Failed: {e}")
            
        # Save
        unreal.EditorAssetLibrary.save_loaded_asset(graph)
        print("Graph Saved!")
        
        # Regen
        world = unreal.EditorLevelLibrary.get_editor_world()
        for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
            comps = actor.get_components_by_class(unreal.PCGComponent)
            if comps:
                comp = comps[0]
                comp.generate_local(True)
                print(f"Regenerated {comp.get_name()}")
                
                # Check Output
                try:
                    data = comp.get_generated_graph_output()
                    if data:
                        total = 0
                        if hasattr(data, 'tagged_data'):
                            print(f"Tagged Data Entries: {len(data.tagged_data)}")
                            for i, td in enumerate(data.tagged_data):
                                pts = []
                                if hasattr(td.data, 'get_points'):
                                    pts = td.data.get_points()
                                count = len(pts)
                                total += count
                                print(f"  Entry {i}: {count} points")
                        print(f"Total Points: {total}")
                except Exception as e: 
                    print(f"Read Error: {e}")

print("\\n=== Done ===")
"""

def debug_volume_output():
    print(f"--- [Debug Volume Output] ---", flush=True)
    
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
    debug_volume_output()
