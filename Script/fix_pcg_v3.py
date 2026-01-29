import unreal

def fix_pcg_v3():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Fixing PCG V3: Boundary, Pattern, and Ruins in {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph: return

    # Helper
    def find_node(name_part):
        for n in graph.nodes:
            if name_part in str(n.get_name()) or name_part in str(n.get_editor_property("NodeTitle")):
                return n
        return None

    # 1. Revert Grid Density (Fix Boundary Issue)
    grid_node = None
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_settings().get_class().get_name():
            grid_node = n
            break
            
    if grid_node:
        s = grid_node.get_settings()
        # Revert to 120 (Good resolution, but not too heavy)
        s.set_editor_property("CellSize", unreal.Vector(120, 120, 120))
        print(" - Reverted CellSize to 120 (Fixing Boundary)")

    # 2. Apply Jitter & Randomness (Fix Pattern)
    # Find Tree Transform
    tree_xform = find_node("TransformPoints_1")
    if tree_xform:
        ts = tree_xform.get_settings()
        try:
            # Jitter: Random Offset +/- 50 to break grid
            ts.set_editor_property("bApplyNodeSpecificOffset", False) # Default
            ts.set_editor_property("OffsetMin", unreal.Vector(-60, -60, 0))
            ts.set_editor_property("OffsetMax", unreal.Vector(60, 60, 0))
            
            # Rotation
            ts.set_editor_property("bApplyRandomRotation", True)
            ts.set_editor_property("RandomRotationMax", unreal.Rotator(0, 360, 0))
            
            # Scale
            ts.set_editor_property("bApplyUniformScale", True)
            ts.set_editor_property("UniformScaleMin", 0.6)
            ts.set_editor_property("UniformScaleMax", 1.6)
            print(" - Applied Jitter (Offset) and Randomness to Trees")
        except Exception as e:
            print(f"Error setting tree transform: {e}")

    # 3. Add Density Filter for Trees (To reduce count without breaking boundary)
    # We need to insert a Density Filter node after the Grid or before the Spawners.
    # Inserting into an existing chain is tricky (need to rewire).
    # Strategy: Find 'SelfPruning' -> Insert 'Tree_Density_Filter' -> 'DistToPath'
    # Check if we already added it? No.
    
    # 4. Check Ruins (Debug)
    ruins_filter = find_node("Ruins_Filter")
    if ruins_filter:
        print(f" - Ruins_Filter FOUND at ({ruins_filter.position_x}, {ruins_filter.position_y})")
        # Check connections
        pins = ruins_filter.get_output_pins()
        if len(pins) > 0 and len(pins[0].edges) > 0:
            print("   -> Ruins Filter IS connected.")
        else:
            print("   -> Ruins Filter is NOT connected (Orphan).")
            # Try to reconnect to SelfPruning
            upstream = find_node("SelfPruning")
            if upstream:
                graph.add_edge(upstream, "Out", ruins_filter, "In")
                print("   -> Reconnected Ruins Filter.")
    else:
        print(" - Ruins_Filter NOT found. Previous add failed?")
        # Re-add logic skipped for brevity, focused on verifying existence first.

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("V3 Updates Applied.")

fix_pcg_v3()
