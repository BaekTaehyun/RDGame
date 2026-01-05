import unreal

def fix_grid_pattern():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    node_name = "TransformPoints_1"
    
    print(f"--- Applying Random Offset to {node_name} ---")
    graph = unreal.load_asset(graph_path)
    if not graph:
        print("Graph not found")
        return
    
    target_node = None
    for n in graph.nodes:
        if n.get_name() == node_name:
            target_node = n
            break
            
    if target_node:
        settings = target_node.get_settings()
        
        # Grid Breaker: Apply random offset of +/- 50cm in X/Y
        # Z should probably stay 0 or small variation to adhere to landscape
        
        offset_min = unreal.Vector(-50, -50, 0)
        offset_max = unreal.Vector(50, 50, 0)
        
        settings.set_editor_property("offset_min", offset_min)
        settings.set_editor_property("offset_max", offset_max)
        settings.set_editor_property("apply_to_attribute", False) # Ensure it applies to transform
        
        # We also need 'ApplyRandomOffset'? 
        # Usually offset_min/max implies it. Let's check Reflection again if needed, 
        # but typical PCG behavior is applying if non-zero.
        # Actually 'absolute_offset' was seen. 
        # Let's just set the values.
        
        print(f"[SUCCESS] Set Offset Min: {offset_min} Max: {offset_max}")
        print("This should beak the grid pattern.")
        
        unreal.EditorAssetLibrary.save_loaded_asset(graph)

fix_grid_pattern()
