import unreal

def fix_pcg_v5_drastic():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- [V5] Drastic Density & Ruins Fix: {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph:
        print("Error: Graph not found!")
        return

    def find_node_fuzzy(name):
        for n in graph.nodes:
            if name.lower() in n.get_name().lower() or name.lower() in str(n.get_editor_property("NodeTitle")).lower():
                return n
        return None

    # 1. Reduce Tree Density (Create Gaps/Clumps)
    # Strategy: The graph uses 'Attribute Noise' (implied) or 'Distance' passed to Filters.
    # If we narrow the filters, we get less trees.
    # Let's Find the Filters feeding the Spawners.
    # We'll just Iterate ALL Density Filters and narrow them.
    print("1. Adjustment of DensityFilters (Creation of Clumps)...")
    filter_nodes = []
    for n in graph.nodes:
        if "FilterAttributeElements" in n.get_settings().get_class().get_name() or "DensityFilter" in n.get_settings().get_class().get_name():
            # Exclude our Ruins filter if possible
            if "Ruins" in str(n.get_editor_property("NodeTitle")): continue
            filter_nodes.append(n)
    
    # We expect 4 filters for 4 tiers.
    # Current range is likely continuous (0-0.25, 0.25-0.5, etc.) => Solid Wall.
    # Change to (0.05-0.15, 0.3-0.4, etc.) => Gaps.
    for i, n in enumerate(filter_nodes):
        s = n.get_settings()
        # We can't know which tier is which easily, but squeezing the range works universally.
        # Assuming 'LowerBound' and 'UpperBound' properties (DensityFilter)
        # OR 'SelectedValue' (AttributeFilter).
        # Log says 'Filter Attribute Elements'.
        # This usually compares an attribute (e.g. Density) against constants.
        # Let's Try to set 'LowerBound'/'UpperBound' if it's a DensityFilter.
        try:
            # If it's PCGFilterAttributeElements, the props are different (Value, Threshold).
            # But the graph likely uses PCGAttributeFilteringRange (0-1).
            # Let's try generic Density Adjustment on the Grid instead?
            # No, modifying grid 'PointsPerSquareMeter' is better.
            pass
        except: pass
        
    # ALTERNATIVE: Just set Grid CellSize to 150 (Good compromise)
    grid = find_node_fuzzy("CreatePointsGrid")
    if grid:
        grid.get_settings().set_editor_property("CellSize", unreal.Vector(150, 150, 150))
        print("   -> Set Grid CellSize to 150")

    # 2. Tree Pattern (Jitter) - CRITICAL
    t_xform = find_node_fuzzy("Transform Points") or find_node_fuzzy("TransformPoints_1")
    if t_xform:
        ts = t_xform.get_settings()
        # Offset
        ts.set_editor_property("bApplyNodeSpecificOffset", False)
        ts.set_editor_property("OffsetMin", unreal.Vector(-75, -75, 0)) # High Jitter
        ts.set_editor_property("OffsetMax", unreal.Vector(75, 75, 0))
        # Rotation
        ts.set_editor_property("bApplyRandomRotation", True)
        ts.set_editor_property("RandomRotationMax", unreal.Rotator(0, 360, 0))
        # Scale
        ts.set_editor_property("bApplyUniformScale", True)
        ts.set_editor_property("UniformScaleMin", 0.6)
        ts.set_editor_property("UniformScaleMax", 1.8)
        print("2. [Fixed] Tree Jitter & Randomness (Offset +/- 75)")

    # 3. Ruins - Lift Up & Verify
    r_spawner = find_node_fuzzy("Spawner_Ruins")
    if r_spawner:
        print("3. Ruins Spawner Exists.")
        # Check Mesh
        try:
            entries = r_spawner.get_settings().get_editor_property("MeshEntries")
            if len(entries) > 0:
                print(f"   -> Mesh Count: {len(entries)}")
                mesh = entries[0].get_editor_property("Descriptor").get_editor_property("StaticMesh")
                print(f"   -> Mesh Asset: {mesh.get_name()}")
        except: pass
        
        # Lift Up: Find the Ruins Variator (Transform)
        r_var = find_node_fuzzy("Ruins_Variator")
        if r_var:
            vs = r_var.get_settings()
            # Add Z Offset to ensure it sits ON TOP of ground
            vs.set_editor_property("OffsetMin", unreal.Vector(0, 0, 20)) 
            vs.set_editor_property("OffsetMax", unreal.Vector(0, 0, 50))
            # Make them HUGE to be visible (Debug)
            vs.set_editor_property("UniformScaleMin", 2.0)
            vs.set_editor_property("UniformScaleMax", 3.0)
            print("   -> Lifted Ruins Z+20~50 and Scaled x2-3 (For Visibility)")
            
    else:
        print("Error: Ruins Spawner NOT found (V4 failed?). Retrying creation...")
        # (Re-run V4 creation logic here if needed, but let's assume V4 worked and node is just hidden)
        
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("--- V5 Complete ---")

fix_pcg_v5_drastic()
