import unreal

def inspect_edge_reflection():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Edge Reflection Inspection: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph: return

    # Traverse to find first edge
    first_edge = None
    for node in graph.nodes:
        if hasattr(node, "input_pins"):
            for pin in node.input_pins:
                if hasattr(pin, "edges") and len(pin.edges) > 0:
                    first_edge = pin.edges[0]
                    break
        if first_edge: break
        
    if not first_edge:
        print("No edges found in graph.")
        return
        
    print(f"Edge Object: {first_edge}")
    
    # Correct way to list properties in generic Python/Unreal
    # 1. Get Class
    cls = first_edge.get_class()
    print(f"Class: {cls.get_name()}")
    
    # 2. Iterate Properties via Unreal Reflection
    print("--- Properties via Unreal Reflection ---")
    # In UE Python, iterating properties often requires iterating FProperty fields of the struct/class
    # If standard 'get_properties' fails on the object strings (which happened before),
    # we can try to guess known names or use dir() carefully.
    
    # Let's try to deduce connections by print
    
    # Known properties in C++ PCGEdge:
    # - InboundNode, OutboundNode (Deprecated?)
    # - InputPin, OutputPin
    # - InboundPin, OutboundPin
    
    candidates = [
        "InboundPin", "OutboundPin", 
        "InputPin", "OutputPin", 
        "InboundNode", "OutboundNode",
        "UpstreamPin", "DownstreamPin"
    ]
    
    for c in candidates:
        try:
            val = first_edge.get_editor_property(c)
            print(f"  [FOUND] {c}: {val}")
        except:
             pass

inspect_edge_reflection()
