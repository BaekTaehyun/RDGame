import unreal

def fix_pcg_v6_verified():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- [V6] Verified Property Fix: {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph:
        print("Error: Graph not found!")
        return

    def find_node_fuzzy(name):
        for n in graph.nodes:
            if name.lower() in n.get_name().lower() or name.lower() in str(n.get_editor_property("NodeTitle")).lower():
                return n
        return None

    # 1. Grid Resolution (Boundary)
    grid = find_node_fuzzy("CreatePointsGrid")
    if grid:
        s = grid.get_settings()
        # Direct property access if available, else set_editor_property
        # 'CellSize' might be 'cell_size' in python? dir() would show. 
        # But set_editor_property("CellSize") worked before (Log: "Reverted Grid CellSize to 100").
        # So we keep using set_editor_property for Grid.
        s.set_editor_property("CellSize", unreal.Vector(100, 100, 100))
        print("1. [Fixed] Grid CellSize = 100")

    # 2. Tree Jitter & Randomness
    t_xform = find_node_fuzzy("Transform Points") or find_node_fuzzy("TransformPoints_1")
    if t_xform:
        ts = t_xform.get_settings()
        # USE VERIFIED ATTRIBUTES
        try:
            # Jitter (Offset)
            ts.offset_min = unreal.Vector(-75, -75, 0)
            ts.offset_max = unreal.Vector(75, 75, 0)
            
            # Rotation (Z Randomness)
            ts.rotation_min = unreal.Rotator(0, 0, 0)
            ts.rotation_max = unreal.Rotator(0, 360, 0)
            
            # Scale
            try:
                ts.uniform_scale = True # Try setting bool
            except: pass
            
            # Even if uniform_scale fails, setting Min/Max to uniform vectors works
            ts.scale_min = unreal.Vector(0.5, 0.5, 0.5)
            ts.scale_max = unreal.Vector(1.8, 1.8, 1.8)
            
            print("2. [Fixed] Tree Jitter/Rotation/Scale applied using Python Attributes.")
        except Exception as e:
            print(f"Error applying Tree Transform: {e}")
    else:
        print("Warning: Tree Transform Node not found")

    # 3. Ruins Layer
    # We will delete old "Ruins_Filter" nodes to prevent duplicates if V4 partially worked,
    # OR just find the existing one and update it.
    
    ruins_filter = find_node_fuzzy("Ruins_Filter")
    if not ruins_filter:
        print("3. [Action] Creating Ruins Layer...")
        # A. Filter
        f_set = unreal.PCGDensityFilterSettings()
        f_set.set_editor_property("LowerBound", 0.8)
        f_set.set_editor_property("UpperBound", 0.9)
        ruins_filter = graph.add_node_instance(f_set)
        ruins_filter.node_title = "Ruins_Filter"
      
        # B. Transform
        x_set = unreal.PCGTransformPointsSettings()
        # Apply attributes immediately
        x_set.rotation_max = unreal.Rotator(0, 360, 0)
        x_set.scale_min = unreal.Vector(2.0, 2.0, 2.0) # BIG Ruins for visibility
        x_set.scale_max = unreal.Vector(3.0, 3.0, 3.0)
        x_set.offset_min = unreal.Vector(0, 0, 10) # Lift up
        x_set.offset_max = unreal.Vector(0, 0, 50)
        
        ruins_var = graph.add_node_instance(x_set)
        ruins_var.node_title = "Ruins_Variator"

        # C. Spawner
        s_set = unreal.PCGStaticMeshSpawnerSettings()
        ruins_spawner = graph.add_node_instance(s_set)
        ruins_spawner.node_title = "Spawner_Ruins"
        
        # Mesh
        mesh_path = "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged"
        mesh_asset = unreal.load_asset(mesh_path)
        if mesh_asset:
             try:
                entries = s_set.get_editor_property("MeshEntries")
                if len(entries) > 0:
                    ent = entries[0]
                    desc = ent.get_editor_property("Descriptor")
                    desc.set_editor_property("StaticMesh", mesh_asset)
             except: pass
        
        # Connect
        upstream = find_node_fuzzy("SelfPruning")
        if upstream:
            graph.add_edge(upstream, "Out", ruins_filter, "In")
            graph.add_edge(ruins_filter, "Out", ruins_var, "In")
            graph.add_edge(ruins_var, "Out", ruins_spawner, "In")
            print("   -> Connected Ruins Layer")
            
        # Position (Try-Except wrapper for safety, though V4 failed on this)
        # We can't set position via python attribute 'position_x' apparently.
        # It's okay, user can arrange it.
    else:
        print("3. [Check] Ruins Layer already exists. (Skipping creation)")

    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("--- V6 Complete ---")

fix_pcg_v6_verified()
