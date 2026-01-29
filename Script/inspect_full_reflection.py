import unreal

def inspect_full_reflection():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Full Reflection Inspection: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph: return

    # Find an edge
    target_edge = None
    for node in graph.nodes:
        if hasattr(node, "input_pins"):
            for pin in node.input_pins:
                if hasattr(pin, "edges") and len(pin.edges) > 0:
                    target_edge = pin.edges[0]
                    break
        if target_edge: break
        
    if not target_edge:
        print("No edges found to inspect.")
        return
        
    print(f"Inspecting Class: {target_edge.get_class().get_name()}")
    
    # Iterate ALL properties in the class
    # Note: get_properties() returns a list of FProperty wrappers
    try:
        props = target_edge.get_class().get_properties()
        print(f"Found {len(props)} properties.")
        
        for p in props:
            p_name = p.get_name()
            # Try to read the value
            val = "<Read Error>"
            try:
                val = target_edge.get_editor_property(p_name)
            except:
                pass
            print(f"  [Property] {p_name} ({p.get_class().get_name()}) = {val}")
            
    except Exception as e:
        print(f"Error iterating properties: {e}")

    # Also check functions just in case
    try:
        funcs = target_edge.get_class().get_functions()
        print(f"Found {len(funcs)} functions.")
        for f in funcs:
            print(f"  [Function] {f.get_name()}")
    except:
        pass

inspect_full_reflection()
