import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"

PYTHON_CODE = """
import unreal

print("=== PCG Graph Connection Analysis ===")

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood"
graph = unreal.load_asset(graph_path)

if not graph:
    print("ERROR: Cannot load graph")
else:
    print(f"Graph: {graph.get_name()}")
    print(f"Total Nodes: {len(graph.nodes)}")
    
    # Categorize nodes
    nodes_by_type = {}
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: 
            continue
        class_name = settings.get_class().get_name()
        if class_name not in nodes_by_type:
            nodes_by_type[class_name] = []
        nodes_by_type[class_name].append(node)
    
    print("\\nNode Types:")
    for t, nodes in nodes_by_type.items():
        print(f"  {t}: {len(nodes)}")
    
    # Find key nodes
    transform_node = None
    filter_nodes = []
    sampler_node = None
    
    for node in graph.nodes:
        settings = node.get_settings()
        if not settings: continue
        class_name = settings.get_class().get_name()
        
        if "PCGTransformPointsSettings" in class_name:
            transform_node = node
        elif "PCGDensityFilterSettings" in class_name:
            filter_nodes.append(node)
        elif "PCGSurfaceSamplerSettings" in class_name:
            sampler_node = node
    
    print(f"\\nTransform Node: {transform_node is not None}")
    print(f"Filter Nodes: {len(filter_nodes)}")
    print(f"Sampler Node: {sampler_node is not None}")
    
    # Check edges (connections)
    print("\\n=== Edge Analysis ===")
    
    # Check what edges exist
    # PCGGraph has 'edges' property? Let's try
    try:
        edges = graph.edges
        print(f"Total Edges: {len(edges)}")
    except:
        print("Cannot access edges directly")
    
    # Try find_edges_between
    if transform_node and filter_nodes:
        print("\\nChecking Transform -> Filter connections:")
        for i, filter_node in enumerate(filter_nodes):
            # Check if there's a connection
            # Unfortunately Python API is limited here
            print(f"  Filter {i}: Cannot verify via Python API")
    
    # Let's try to connect Transform -> first Filter
    if transform_node and filter_nodes:
        print("\\n=== Attempting to Connect Transform -> Filters ===")
        first_filter = filter_nodes[0]
        
        try:
            success = graph.add_edge(transform_node, "Out", first_filter, "In")
            print(f"Connection 1 result: {success}")
            
            # Connect to all filters
            for i, f in enumerate(filter_nodes):
                try:
                    s = graph.add_edge(transform_node, "Out", f, "In")
                    print(f"  Filter {i}: Connected = {s}")
                except Exception as e:
                    print(f"  Filter {i}: Error = {e}")
            
            # Save
            unreal.EditorAssetLibrary.save_loaded_asset(graph)
            print("\\nGraph Saved!")
            
        except Exception as e:
            print(f"Connection Error: {e}")

print("\\n=== Done ===")
"""

def fix_connections():
    print(f"--- [Fix PCG Connections] ---", flush=True)
    
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
    fix_connections()
