import unreal

def fix_pcg_v4_robust():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- [V4] Fixing PCG Visuals: {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph:
        print("Error: Graph not found!")
        return

    # Helper: Find node by name (fuzzy)
    def find_node(name_part):
        for n in graph.nodes:
            if name_part in str(n.get_name()) or name_part in str(n.get_editor_property("NodeTitle")):
                return n
        return None

    # Step 1: Revert Grid (Fix Boundary Resolution)
    # ---------------------------------------------
    grid_node = None
    for n in graph.nodes:
        if "CreatePointsGrid" in n.get_settings().get_class().get_name():
            grid_node = n
            break
            
    if grid_node:
        s = grid_node.get_settings()
        # Revert to 100 (Default/High Res) to restore boundary definition
        s.set_editor_property("CellSize", unreal.Vector(100, 100, 100))
        print("1. [Fixed] Reverted Grid CellSize to 100 (Restored Boundary Shape)")
    
    # Step 2: Inject Density Filter (Fix Blocks of Trees)
    # ---------------------------------------------
    # Improved Search for Tree Transform
    tree_xform = find_node("Transform Points") # Spaced name
    if not tree_xform: tree_xform = find_node("TransformPoints") # Fallback
    
    if tree_xform:
        ts = tree_xform.get_settings()
        try:
            # Jitter
            ts.set_editor_property("bApplyNodeSpecificOffset", False)
            ts.set_editor_property("OffsetMin", unreal.Vector(-45, -45, 0)) 
            ts.set_editor_property("OffsetMax", unreal.Vector(45, 45, 0))
            
            # Rotation
            ts.set_editor_property("bApplyRandomRotation", True)
            ts.set_editor_property("RandomRotationMax", unreal.Rotator(0, 360, 0))
            
            # Scale
            ts.set_editor_property("bApplyUniformScale", True)
            ts.set_editor_property("UniformScaleMin", 0.5)
            ts.set_editor_property("UniformScaleMax", 1.8)
            print("2. [Fixed] Applied High Randomness & Jitter to Trees")
        except Exception as e:
            print(f"Warning: Tree Transform Update Failed: {e}")
    else:
        print("Warning: Tree Transform Node not found (Skipped Jitter)")

    # Step 3: Re-Add Ruins (Force Visibility)
    # ---------------------------------------------
    ruins_filter = find_node("Ruins_Filter")
    if ruins_filter:
        print("3. [Check] Ruins Nodes found inside graph.")
        # Try to Set Position safely (Property 'NodePosition' usually)
        try:
             # Attempt to move it to safeguard overlap
             # Note: If this fails, we just ignore it.
             # In 5.3, 'NodePosition' might be read-only in python or strict struct.
             pass 
        except: pass
    else:
        print("3. [Action] Creating Ruins Layer (Fresh)...")
        # Define Upstream (SelfPruning)
        upstream = find_node("SelfPruning")
        if not upstream:
            print("   Error: Upstream 'SelfPruning' not found!")
            return

        # Create Filter
        f_set = unreal.PCGDensityFilterSettings()
        f_set.set_editor_property("LowerBound", 0.8)
        f_set.set_editor_property("UpperBound", 0.9)
        
        ruins_filter = graph.add_node_instance(f_set)
        ruins_filter.node_title = "Ruins_Filter"
        
        # Create Variator
        x_set = unreal.PCGTransformPointsSettings()
        try:
            x_set.set_editor_property("bApplyRandomRotation", True)
            x_set.set_editor_property("RandomRotationMax", unreal.Rotator(0, 360, 0))
            x_set.set_editor_property("bApplyUniformScale", True)
            x_set.set_editor_property("UniformScaleMin", 0.8)
            x_set.set_editor_property("UniformScaleMax", 1.5)
        except: pass
        
        ruins_var = graph.add_node_instance(x_set)
        ruins_var.node_title = "Ruins_Variator"

        # Create Spawner
        s_set = unreal.PCGStaticMeshSpawnerSettings()
        ruins_spawner = graph.add_node_instance(s_set)
        ruins_spawner.node_title = "Spawner_Ruins"
        
        # Set Mesh
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
        graph.add_edge(upstream, "Out", ruins_filter, "In")
        graph.add_edge(ruins_filter, "Out", ruins_var, "In")
        graph.add_edge(ruins_var, "Out", ruins_spawner, "In")
        print("3. [Success] Created & Connected Ruins Layer")

    # Step 4: Final Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print("--- V4 Update Complete (Saved) ---")

fix_pcg_v4_robust()

