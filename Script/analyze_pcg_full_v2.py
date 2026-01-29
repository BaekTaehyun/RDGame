import unreal

def analyze_full():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Analyzing Graph: {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph: return

    print(f"\n[1. Node List] (Total: {len(graph.nodes)})")
    nodes_by_id = {}
    for n in graph.nodes:
        # Title fallback
        title = "None"
        try: title = n.get_editor_property("NodeTitle")
        except: pass
        
        # Name
        name = n.get_name()
        nodes_by_id[name] = title
        
        # Position?
        pos_str = "?"
        try: 
            # 5.3 might use 'NodePosition' struct
            pos = n.get_editor_property("NodePosition")
            pos_str = f"({pos.x}, {pos.y})"
        except: pass
        
        print(f" - [{name}] Title='{title}' Pos={pos_str} Class={n.get_settings().get_class().get_name()}")

    print("\n[2. Connectivity Check]")
    # We Iterate NODES and their INPUT PINS to find what is connected to them.
    # OR Output Pins.
    # Unreal Python API for PCG connectivity is tricky.
    # Usually: Node -> GetOutputPins -> Pin -> Edges -> TargetPin -> TargetNode
    
    found_ruins_chain = False
    
    for n in graph.nodes:
        name = n.get_name()
        for pin in n.get_output_pins():
            label = pin.get_editor_property("Label")
            if len(pin.edges) > 0:
                for edge in pin.edges:
                    # Target
                    # In 5.3 Edge has 'inbound_node'/'outbound_node' or 'input_pin'/'output_pin'?
                    # Let's inspect the edge object via DIR if needed, but assuming standard print works.
                    # Actually valid python properties for PCGEdge are 'InputPin' and 'OutputPin' usually.
                    # BUT 'InputPin' is the pin on the *downstream* node (the Input OF the downstream).
                    down_node = None
                    try:
                        down_pin = edge.input_pin # The pin receiving the connection
                        down_node = down_pin.node
                    except: pass
                    
                    if down_node:
                        d_name = down_node.get_name()
                        d_title = nodes_by_id.get(d_name, "Unknown")
                        print(f"   [Edge] {nodes_by_id[name]} ({label})  --->  {d_title}")
                        
                        if "Ruins" in str(d_title) or "Ruins" in str(nodes_by_id[name]):
                            found_ruins_chain = True

    print("\n[3. Key Settings]")
    # Grid
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_settings().get_class().get_name():
            print(f" - Grid CellSize: {n.get_settings().get_editor_property('CellSize')}")
            
    # Transform
    for n in graph.nodes:
        if "TransformPoints" in n.get_settings().get_class().get_name():
            s = n.get_settings()
            try:
                print(f" - Transform {n.get_editor_property('NodeTitle')}:")
                print(f"     Offset Min/Max: {s.offset_min} / {s.offset_max}")
                print(f"     Rot Min/Max: {s.rotation_min} / {s.rotation_max}")
            except: pass

    if not found_ruins_chain:
        print("\n!!! WARNING: NO EDGES CONNECTED TO RUINS NODES FOUND !!!")
    else:
        print("\n[OK] Ruins chain connectivity detected.")

analyze_full()
