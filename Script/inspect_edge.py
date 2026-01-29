import unreal

def inspect_edge():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Edge Inspection: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph: return

    for node in graph.nodes:
        # Check input pins for any edges
        if hasattr(node, "input_pins"):
            for pin in node.input_pins:
                if hasattr(pin, "edges") and len(pin.edges) > 0:
                    edge = pin.edges[0]
                    print(f"--- Found Edge on Node '{node.get_name()}' ---")
                    print(f"Edge: {edge}")
                    print(f"Edge Type: {type(edge)}")
                    
                    print("--- Edge Attributes (dir) ---")
                    for d in dir(edge):
                        if not d.startswith("_"):
                            print(f"  {d}")
                            
                    # Try accessing common properties
                    print("--- Property Values ---")
                    calc_props = ["InboundNode", "OutboundNode", "InputPin", "OutputPin", "UpstreamPin", "DownstreamPin"]
                    for p in calc_props:
                        if hasattr(edge, p):
                            print(f"  {p}: {getattr(edge, p)}")
                            
                    return # Just inspect one edge

inspect_edge()
