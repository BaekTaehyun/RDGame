import unreal

def inspect_selector():
    print(">>> Inspecting PCG Spawner Mesh Selector")
    
    # Create temp graph
    factory = unreal.PCGGraphFactory()
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset("Temp_Inspector", "/Game", unreal.PCGGraph, factory)
    
    if not graph:
        print("Failed to create temp graph")
        return

    try:
        # Add Spawner
        node = graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
        if isinstance(node, (tuple, list)): node = node[0]
        
        settings = node.get_settings()
        instance = settings.mesh_selector_instance
        
        print(f"Selector Class: {instance.get_class().get_name()}")
        
        # Check standard property names
        candidates = ["MeshEntries", "Entries", "Meshes", "WeightedMeshEntries"]
        
        found = False
        for c in candidates:
            try:
                val = instance.get_editor_property(c)
                print(f"[O] Found Property '{c}': {val}")
                found = True
            except:
                pass
                
        if not found:
            print("--- All Properties on Selector ---")
            for prop in instance.get_class().get_properties():
                print(f"  {prop.get_name()}")
                
    finally:
        # Cleanup
        unreal.EditorAssetLibrary.delete_asset(graph.get_path_name())

inspect_selector()
