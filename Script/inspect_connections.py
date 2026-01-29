import unreal

def inspect_connections():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Connection Inspection: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph:
        return

    nodes = graph.nodes
    if not nodes:
        print("No nodes found.")
        return

    node = nodes[0]
    print(f"Testing Node: {node.get_name()}")
    
    # helper
    def check_attr(obj, attr_name):
        if hasattr(obj, attr_name):
            try:
                val = getattr(obj, attr_name)
                print(f"  [YES] {attr_name}: {type(val)} - {val}")
                return val
            except Exception as e:
                print(f"  [YES] {attr_name} (Error reading): {e}")
        else:
            print(f"  [NO]  {attr_name}")
            
    # Check Common API patterns for Pins
    print("\n--- Checking Pin Attributes ---")
    check_attr(node, "input_pins")
    check_attr(node, "output_pins")
    check_attr(node, "get_input_pins")
    check_attr(node, "get_output_pins")
    check_attr(node, "pins")
    
    # Check Connections/Edges
    print("\n--- Checking Edge Attributes ---")
    check_attr(node, "edges")
    check_attr(node, "get_inbound_edges")
    check_attr(node, "get_outbound_edges")
    
    # Check Graph Edges
    print("\n--- Checking Graph Edge Attributes ---")
    check_attr(graph, "edges")
    check_attr(graph, "get_all_edges")

inspect_connections()
