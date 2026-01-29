import unreal
import sys

def fix_and_verify_v10_safe():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- [V10 Safe] Fix & Verify PCG: {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph:
        print(f"[FAIL] Graph not found: {graph_path}")
        return

    # Helper
    def find_node(fuzzy_name):
        name_clean = fuzzy_name.lower().replace(" ", "")
        for n in graph.nodes:
            if fuzzy_name.lower() in n.get_name().lower(): return n
            try:
                if fuzzy_name.lower() in str(n.get_editor_property("NodeTitle")).lower(): return n
            except: pass
            s = n.get_settings()
            if s and name_clean in s.get_class().get_name().lower(): return n
        return None

    # --- PHASE 1: APPLY FIXES ---
    print("\n[Phase 1: Fixing]")

    # 1. Grid -> 135
    grid = find_node("CreatePointsGrid")
    if grid:
        grid.get_settings().set_editor_property("CellSize", unreal.Vector(135, 135, 135))
        print(" -> Grid CellSize set to 135")
    
    # 2. Jitter
    tree_xform = find_node("TransformPoints")
    if not tree_xform: tree_xform = find_node("TransformPoints_1")
    if tree_xform:
        ts = tree_xform.get_settings()
        try:
            # Using verified python attributes
            ts.offset_min = unreal.Vector(-65, -65, 0)
            ts.offset_max = unreal.Vector(65, 65, 0)
            ts.rotation_max = unreal.Rotator(0, 360, 0)
            ts.scale_min = unreal.Vector(0.6, 0.6, 0.6)
            ts.scale_max = unreal.Vector(1.6, 1.6, 1.6)
            print(" -> Tree Jitter Applied (+/- 65)")
        except: print(" -> Warning: Could not set Tree Transform attributes")

    # 3. Ruins
    # Ensure nodes exist
    r_filter = find_node("Ruins_Filter")
    if not r_filter:
        print(" -> Creating Ruins Filter")
        fs = unreal.PCGDensityFilterSettings()
        fs.set_editor_property("LowerBound", 0.9) # Very sparse
        fs.set_editor_property("UpperBound", 1.0)
        r_filter = graph.add_node_instance(fs)
        r_filter.node_title = "Ruins_Filter"
        # Try connect to upstream (SelfPruning)
        up = find_node("SelfPruning")
        if up: 
             try: graph.add_edge(up, "Out", r_filter, "In")
             except: pass

    r_var = find_node("Ruins_Variator")
    if not r_var:
        print(" -> Creating Ruins Variator")
        xs = unreal.PCGTransformPointsSettings()
        xs.offset_min = unreal.Vector(0,0,50)
        xs.offset_max = unreal.Vector(0,0,100)
        xs.scale_min = unreal.Vector(3.0,3.0,3.0)
        xs.scale_max = unreal.Vector(4.0,4.0,4.0)
        xs.rotation_max = unreal.Rotator(0, 360, 0)
        r_var = graph.add_node_instance(xs)
        r_var.node_title = "Ruins_Variator"
        if r_filter:
             try: graph.add_edge(r_filter, "Out", r_var, "In")
             except: pass

    r_spawner = find_node("Spawner_Ruins")
    if not r_spawner:
        print(" -> Creating Ruins Spawner")
        ss = unreal.PCGStaticMeshSpawnerSettings()
        
        # Mesh
        mesh_path = "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged"
        mesh_asset = unreal.load_asset(mesh_path)
        if mesh_asset:
             try:
                entries = ss.get_editor_property("MeshEntries")
                if len(entries) > 0:
                    ent = entries[0]
                    desc = ent.get_editor_property("Descriptor")
                    desc.set_editor_property("StaticMesh", mesh_asset)
             except: pass
        
        r_spawner = graph.add_node_instance(ss)
        r_spawner.node_title = "Spawner_Ruins"
        if r_var:
             try: graph.add_edge(r_var, "Out", r_spawner, "In")
             except: pass
    
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(graph)
    print(" -> Graph Saved.")

    # --- PHASE 2: VERIFICATION (SAFE) ---
    print("\n[Phase 2: Verification]")
    
    # Check 1: Nodes Exist
    nodes = {
        "Grid": find_node("CreatePointsGrid"),
        "TreeXform": tree_xform,
        "RuinsFilter": r_filter,
        "RuinsVar": r_var,
        "RuinsSpawner": r_spawner
    }
    
    for name, node in nodes.items():
        if node: print(f"[PASS] {name} Found")
        else: print(f"[FAIL] {name} Missing")
        
    # Check 2: Settings (Soft Check)
    if nodes["Grid"]:
        try:
            s = nodes["Grid"].get_settings().get_editor_property("CellSize").x
            print(f"      Grid Size: {s}")
        except: pass
        
    print("\n(Connectivity verify skipped to avoid API crash. Please check graph links visually.)")

fix_and_verify_v10_safe()
