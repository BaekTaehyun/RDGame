import unreal
import pcg_visual_tools
import imp
imp.reload(pcg_visual_tools)

def run_v9():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print("--- Running V9 (Final Repair) ---")
    
    tools = pcg_visual_tools.PCGVisualTools(graph_path)
    
    # 1. Cleanup Ineffective Filter
    t_filter = tools.find_node("Tree_Reduction_Filter")
    if t_filter:
        print("Removing ineffective Tree_Reduction_Filter...")
        # We need to bridge the connection: Up -> Down
        # But for now, let's just bypass it or leave it open?
        # Re-wiring in python is risky without explicit edge struct access.
        # EASIER: Just set its range to 0.0-1.0 (Pass-through) so it does no harm.
        s = t_filter.get_settings()
        s.set_editor_property("LowerBound", 0.0)
        s.set_editor_property("UpperBound", 1.0)
        print(" -> Set Tree Filter to Pass-Through (0.0-1.0)")
        
    # 2. Adjust Density via Grid (Reliable)
    grid = tools.find_node("CreatePointsGrid")
    if grid:
        s = grid.get_settings()
        # 100 = Dense, 180 = Sparse/Blocky. 135 = Sweet Spot?
        s.set_editor_property("CellSize", unreal.Vector(135, 135, 135))
        print(" -> Set Grid CellSize to 135 (Moderate Density)")

    # 3. Fix Tree Jitter
    # High Jitter to break the 135 grid pattern
    tools.apply_transform("Transform Points", offset=65, rotation=True, scale_min=0.6, scale_max=1.6)

    # 4. Complete Ruins Chain (Missing Nodes)
    # Check what exists
    r_filter = tools.find_node("Ruins_Filter")
    r_var = tools.find_node("Ruins_Variator")
    r_spawner = tools.find_node("Spawner_Ruins") # Use fuzzy
    
    # Ensure Filter Exists
    if not r_filter:
        print("Re-creating Ruins Filter...")
        f_set = unreal.PCGDensityFilterSettings()
        f_set.set_editor_property("LowerBound", 0.9) # Very sparse
        f_set.set_editor_property("UpperBound", 1.0)
        r_filter = tools.graph.add_node_instance(f_set)
        r_filter.node_title = "Ruins_Filter"
        
        # Connect to SelfPruning
        up = tools.find_node("SelfPruning")
        if up: tools.graph.add_edge(up, "Out", r_filter, "In")

    # Ensure Variator Exists
    if not r_var:
        print("Creating Missing Ruins Variator...")
        x_set = unreal.PCGTransformPointsSettings()
        # Setup visuals (Huge & Lifted)
        x_set.offset_min = unreal.Vector(0, 0, 50)
        x_set.offset_max = unreal.Vector(0, 0, 100)
        x_set.scale_min = unreal.Vector(3.0, 3.0, 3.0)
        x_set.scale_max = unreal.Vector(4.0, 4.0, 4.0)
        x_set.rotation_max = unreal.Rotator(0, 360, 0)
        
        r_var = tools.graph.add_node_instance(x_set)
        r_var.node_title = "Ruins_Variator"
        
        # Connect Filter -> Variator
        tools.graph.add_edge(r_filter, "Out", r_var, "In")
        
    # Ensure Spawner Exists
    if not r_spawner:
        print("Creating Missing Ruins Spawner...")
        s_set = unreal.PCGStaticMeshSpawnerSettings()
        
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
        
        r_spawner = tools.graph.add_node_instance(s_set)
        r_spawner.node_title = "Spawner_Ruins"
        
        # Connect Variator -> Spawner
        tools.graph.add_edge(r_var, "Out", r_spawner, "In")

    tools.save()
    print(f"V9 Complete. Total Nodes: {len(tools.graph.nodes)} (Should be ~20)")

run_v9()
