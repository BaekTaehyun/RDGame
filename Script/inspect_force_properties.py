import unreal

def inspect_force():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Force Inspection: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph: return

    # Find edge
    target_edge = None
    for node in graph.nodes:
        if hasattr(node, "input_pins"):
            for pin in node.input_pins:
                if hasattr(pin, "edges") and len(pin.edges) > 0:
                    target_edge = pin.edges[0]
                    break
        if target_edge: break
    
    if not target_edge:
        print("No edge found.")
        return
        
    print(f"Edge: {target_edge}")
    
    # Brute Force List
    candidates = [
        "InboundPin", "OutboundPin",
        "InputPin", "OutputPin",
        "UpstreamPin", "DownstreamPin",
        "SourcePin", "DestPin",
        "FromPin", "ToPin",
        "InboundNode", "OutboundNode",
        "InputNode", "OutputNode",
        "UpstreamNode", "DownstreamNode",
        "SourceNode", "TargetNode",
        "InboundLabel", "OutboundLabel",
        "Data"
    ]
    
    print("--- Brute Force Property Check ---")
    for c in candidates:
        try:
            val = target_edge.get_editor_property(c)
            # If we get here, it exists!
            print(f"  [SUCCESS] {c}: {val}")
        except:
            # print(f"  [Fail] {c}")
            pass
            
    print("--- Class Dict (if available) ---")
    try:
        # Sometimes class.__dict__ helps
        print(dir(target_edge.get_class()))
    except:
        pass

inspect_force()
