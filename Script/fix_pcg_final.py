import unreal

def fix_pcg_final():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Applying Final Visual Fixes to {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph:
        print("Error: Graph not found.")
        return

    def find_node(name_part):
        for n in graph.nodes:
            # Cast to string safely
            node_name = str(n.get_name())
            try:
                node_title = str(n.get_editor_property("NodeTitle"))
            except:
                node_title = ""
            
            if name_part in node_name or name_part in node_title:
                return n
        return None

    # --- 1. Reduce Density (Grid Cell Size) ---
    grid_node = None
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_settings().get_class().get_name():
            grid_node = n
            break
    
    if grid_node:
        print(f"Found Grid Node: {grid_node.get_name()}")
        s = grid_node.get_settings()
        # Set CellSize to 400 (Significant reduction in density)
        new_size = unreal.Vector(400, 400, 400)
        s.set_editor_property("CellSize", new_size)
        print(f" - Set CellSize to {new_size} (Much Lower Density)")

    # --- 2. Add Ruins Layer (if missing) ---
    ruins_filter = find_node("Ruins_Filter")
    
    if not ruins_filter:
        print("Adding Ruins Layer...")
        # A. Filter
        filter_settings = unreal.PCGDensityFilterSettings()
        filter_settings.set_editor_property("LowerBound", 0.6)
        filter_settings.set_editor_property("UpperBound", 0.7)
        
        ruins_filter = graph.add_node_instance(filter_settings)
        ruins_filter.node_title = "Ruins_Filter"
        # Position it visually below the existing graph
        ruins_filter.position_x = 500
        ruins_filter.position_y = 1000

        # B. Transform
        xform_settings = unreal.PCGTransformPointsSettings()
        try:
            xform_settings.set_editor_property("bApplyRandomRotation", True)
            xform_settings.set_editor_property("RandomRotationMin", unreal.Rotator(0, 0, 0))
            xform_settings.set_editor_property("RandomRotationMax", unreal.Rotator(0, 360, 0))
            
            xform_settings.set_editor_property("bApplyUniformScale", True)
            xform_settings.set_editor_property("UniformScaleMin", 0.8)
            xform_settings.set_editor_property("UniformScaleMax", 1.5)
        except: pass
        
        ruins_xform = graph.add_node_instance(xform_settings)
        ruins_xform.node_title = "Ruins_Variator"
        ruins_xform.position_x = 800
        ruins_xform.position_y = 1000

        # C. Spawner
        spawner_settings = unreal.PCGStaticMeshSpawnerSettings()
        mesh_path = "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged"
        mesh_asset = unreal.load_asset(mesh_path)
        if mesh_asset:
            entries = spawner_settings.get_editor_property("MeshEntries")
            if len(entries) > 0:
                ent = entries[0]
                desc = ent.get_editor_property("Descriptor")
                desc.set_editor_property("StaticMesh", mesh_asset)
        
        ruins_spawner = graph.add_node_instance(spawner_settings)
        ruins_spawner.node_title = "Spawner_Ruins"
        ruins_spawner.position_x = 1100
        ruins_spawner.position_y = 1000
        
        # Connect
        upstream = find_node("SelfPruning")
        if upstream:
            # Note: add_edge takes (UpstreamNode, Name, DownstreamNode, Name)
            graph.add_edge(upstream, "Out", ruins_filter, "In")
            graph.add_edge(ruins_filter, "Out", ruins_xform, "In")
            graph.add_edge(ruins_xform, "Out", ruins_spawner, "In")
            print(" - Connected Ruins Layer (Visible at Y=1000)")
        else:
            print("Error: SelfPruning node not found, cannot connect Ruins.")

    # --- 3. Fix Tree Randomness ---
    # Find TransformPoints_1 (Tree)
    tree_xform = find_node("TransformPoints_1") # Assuming name
    # Or navigate via logic (First spawner's upstream)
    if tree_xform:
        tx_set = tree_xform.get_settings()
        try:
            tx_set.set_editor_property("bApplyRandomRotation", True)
            tx_set.set_editor_property("RandomRotationMin", unreal.Rotator(0, 0, 0))
            tx_set.set_editor_property("RandomRotationMax", unreal.Rotator(0, 360, 0))
            
            tx_set.set_editor_property("bApplyUniformScale", True)
            tx_set.set_editor_property("UniformScaleMin", 0.7) # More variance
            tx_set.set_editor_property("UniformScaleMax", 1.4)
            print(" - Applied Tree Randomness")
        except Exception as e:
            print(f"Warning fixing tree randomness: {e}")

    # --- 4. SAVE ---
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("Graph Saved. Please check Editor.")

fix_pcg_final()
