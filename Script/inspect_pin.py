import unreal

def inspect_pin():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Pin Inspection: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph or not graph.nodes:
        return

    node = graph.nodes[0]
    if not node.input_pins:
        print("Node 0 has no input pins.")
        return
        
    pin = node.input_pins[0]
    print(f"Pin: {pin}")
    print(f"Pin Type: {type(pin)}")
    
    print("--- Pin Attributes (dir) ---")
    for d in dir(pin):
        if not d.startswith("_"):
            print(f"  {d}")
            
    print("--- Pin Properties ---")
    try:
        for p in pin.get_class().get_properties():
            p_name = p.get_name()
            val = "?"
            try: val = pin.get_editor_property(p_name)
            except: pass
            print(f"  {p_name}: {val}")
            
            # If we find Edges, let's peek at them
            if p_name == "Edges":
                if val:
                    print(f"    Edge Count: {len(val)}")
                    if len(val) > 0:
                        edge = val[0]
                        print(f"    Edge 0: {edge}")
                        print(f"    Edge Type: {type(edge)}")
    except Exception as e:
        print(f"Error inspecting properties: {e}")

inspect_pin()
