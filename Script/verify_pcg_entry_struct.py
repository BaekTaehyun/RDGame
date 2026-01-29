import unreal

def verify_entry():
    print(">>> Verifying PCG Mesh Entry Struct")
    
    factory = unreal.PCGGraphFactory()
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset("Temp_Entry_Test", "/Game", unreal.PCGGraph, factory)
    
    try:
        node = graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
        if isinstance(node, (tuple, list)): node = node[0]
        settings = node.get_settings()
        
        # Get Selector
        sel = None
        if hasattr(settings, "mesh_selector_parameters"):
            sel = settings.mesh_selector_parameters
        else:
            sel = settings.mesh_selector_instance
            
        print(f"Selector: {sel}")
        
        # Get Entries
        entries = sel.get_editor_property("MeshEntries")
        print(f"Current Entries Type: {type(entries)}")
        print(f"Current Entries: {entries}")
        
        # Check Class Availability
        candidates = ["PCGStaticMeshSpawnerEntry", "PCGMeshSelectorWeightedEntry"]
        
        valid_cls = None
        for c in candidates:
            cls = getattr(unreal, c, None)
            if cls:
                print(f"[O] Found class 'unreal.{c}'")
                valid_cls = cls
            else:
                print(f"[X] 'unreal.{c}' not found")
        
        if valid_cls:
            print(f"Attempting to instantiate {valid_cls.__name__}...")
            try:
                inst = valid_cls()
                print(f"[O] Instantiated: {inst}")
                
                # Check properties
                print(f"Properties of {valid_cls.__name__}:")
                for p in inst.get_class().get_properties():
                    print(f"  {p.get_name()} ({p.get_class().get_name()})")
                
                # Try setting mesh
                mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
                if "descriptor" in dir(inst):
                   # Newer versions use descriptor struct?
                   pass
                
                if hasattr(inst, "mesh"):
                    inst.mesh = mesh
                    print("[O] Set 'mesh' property")
                elif hasattr(inst, "static_mesh"):
                    inst.static_mesh = mesh
                    print("[O] Set 'static_mesh' property")
                
            except Exception as e:
                print(f"[X] Instantiation failed: {e}")

    finally:
        unreal.EditorAssetLibrary.delete_asset(graph.get_path_name())

verify_entry()
