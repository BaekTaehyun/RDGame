import unreal

def inspect_api_and_pin():
    print("--- Inspecting AssetTools API ---")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    print(f"AssetTools Type: {type(tools)}")
    
    # Check for export methods
    for d in dir(tools):
        if "export" in d.lower():
            print(f"  Method: {d}")
            
    print("\n--- Inspecting PCGPin Properties (Brute Force) ---")
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    graph = unreal.load_asset(graph_path)
    if not graph: return
    
    # Find a pin with edges
    target_pin = None
    for node in graph.nodes:
        if hasattr(node, "input_pins"):
            for pin in node.input_pins:
                if hasattr(pin, "edges") and len(pin.edges) > 0:
                    target_pin = pin
                    break
        if target_pin: break
        
    if not target_pin:
        print("No connected pin found.")
        return
        
    print(f"Pin: {target_pin.get_name()}")
    
    # Candidates for connection info
    candidates = [
        "Edges", "ConnectedPins", "Links", "LinkedTo", "ConnectedTo",
        "SourcePin", "TargetPin", "OtherPin", "Connection"
    ]
    
    for c in candidates:
        try:
            val = target_pin.get_editor_property(c)
            print(f"  [SUCCESS] {c}: {val}")
        except:
            pass

inspect_api_and_pin()
