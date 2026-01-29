import unreal

def inspect_structure():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Inspecting Structure: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph:
        print("Graph not found.")
        return

    print(f"Graph: {graph}")
    
    # Check Nodes
    nodes = graph.nodes
    print(f"Node Count: {len(nodes)}")
    
    if len(nodes) > 0:
        node = nodes[0]
        print(f"--- Node 0 Inspection ---")
        print(f"Type: {type(node)}")
        
        # Name variations
        print(f"get_name(): {node.get_name()}")
        print(f"get_fname(): {node.get_fname()}")
        print(f"get_path_name(): {node.get_path_name()}")
        
        # Dir
        print("--- Attributes (dir) ---")
        for x in list(dir(node))[:20]: # Show first 20
            if not x.startswith("_"):
                print(f"  {x}")
                
        # Try finding Edges or Pins via Properties
        print("--- Editor Properties ---")
        try:
            props = node.get_class().get_properties()
            for p in props:
                p_name = p.get_name()
                val = "?"
                try: val = node.get_editor_property(p_name)
                except: pass
                # Clean up output for objects
                print(f"  {p_name}: {val}")
        except:
            print("  Could not iterate properties")

inspect_structure()
