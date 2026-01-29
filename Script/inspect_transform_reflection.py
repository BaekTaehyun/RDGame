import unreal

def inspect_reflection():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    node_name = "TransformPoints_1"
    
    print(f"--- Reflection: {node_name} ---")
    graph = unreal.load_asset(graph_path)
    if not graph: return
    
    target_node = None
    for n in graph.nodes:
        if n.get_name() == node_name:
            target_node = n
            break
            
    if target_node:
        settings = target_node.get_settings()
        print(f"Class: {settings.get_class().get_name()}")
        
        # List all properties
        for p in dir(settings):
            if not p.startswith('_') and not p[0].isupper(): # Python exposed often lower_case_with_underscore
                # Actually, dir() usually returns snake_case for properties in Unreal Python
                pass
                
        # Brute force property listing via helper if available, or just dir()
        # Let's print everything in dir() that looks like a property
        print("Properties found in dir():")
        for d in dir(settings):
             if "rot" in d.lower() or "scale" in d.lower() or "apply" in d.lower():
                 try:
                     val = getattr(settings, d)
                     print(f"  {d}: {val}")
                 except:
                     pass

inspect_reflection()
