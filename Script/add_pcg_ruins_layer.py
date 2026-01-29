import unreal

def add_ruins_layer():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    graph = unreal.load_asset(graph_path)
    if not graph:
        print(f"Graph not found: {graph_path}")
        return

    # 1. Define Assets
    ruin_mesh_path = "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged"
    ruin_mesh = unreal.load_asset(ruin_mesh_path)
    if not ruin_mesh:
        print(f"Mesh not found: {ruin_mesh_path}")
        return

    # 2. Find Upstream Connection (e.g. Density Filter or Difference node)
    # Strategy: Find the node named "Difference_1" or "Tier4_Filter" (from previous analysis)
    # Or just find the main 'Difference' node that subtracts path from noise.
    # For now, I'll attach to the 'Input' -> 'SurfaceSampler' -> 'TransformPoints' chain?
    # Better: Analyze finding "Difference_0".
    
    upstream_node = None
    for n in graph.nodes:
        if "Difference" in n.get_name(): 
            upstream_node = n
            break
    
    if not upstream_node:
        print("Could not find upstream 'Difference' node to attach to.")
        return

    print(f"Attaching Ruins Layer to: {upstream_node.get_name()}")

    # 3. Create Nodes
    # A. Density Filter (Only spawn ruins in sparse areas? or specific noise range)
    filter_node = graph.add_node(unreal.PCGDensityFilterSettings)
    filter_node.node_title = "Ruins_Filter"
    # Set threshold to be rare (e.g. 0.8 - 0.9)
    # Note: Settings need to be set via 'get_settings()'
    f_set = filter_node.get_settings()
    # f_set.lower_bound = 0.8 # Python API for struct Props might be tricky, using set_editor_property
    f_set.set_editor_property("LowerBound", 0.6)
    f_set.set_editor_property("UpperBound", 0.7) # Narrow band for rarity

    # B. Transform Points
    xform_node = graph.add_node(unreal.PCGTransformPointsSettings)
    xform_node.node_title = "Ruins_Variator"
    x_set = xform_node.get_settings()
    # Apply Rotation 0-360 Z
    # Apply Scale 0.8-1.5 Uniform
    x_set.set_editor_property("ApplyRandomRotation", True)
    # Scale... need detailed property access or use default randomness
    
    # C. Spawner
    spawner_node = graph.add_node(unreal.PCGStaticMeshSpawnerSettings)
    spawner_node.node_title = "Spawner_Ruins"
    s_set = spawner_node.get_settings()
    
    # Set Mesh Entry
    # This part is complex in Python. We use 'set_smart_property' logic or standard API.
    # In 5.3+, 'MeshEntries' is an array of PCGStaticMeshSpawnerEntry.
    # We will try to add an entry.
    try:
        # Construct Entry? No, it's a struct.
        # We might need to use the MCP helper 'set_pcg_node_properties' logic if direct python fails.
        # But let's try direct manipulation of the array.
        entries = s_set.get_editor_property("MeshEntries") # It's a list (BP accessible)
        # However, creating a new struct 'PCGStaticMeshSpawnerEntry' might not be exposed.
        # Workaround: The default usually has one entry. modifying it.
        if len(entries) > 0:
            entry = entries[0]
            desc = entry.get_editor_property("Descriptor")
            desc.set_editor_property("StaticMesh", ruin_mesh)
        else:
            print("Warning: Spawner has no default entries to modify.")
    except Exception as e:
        print(f"Error setting mesh: {e}")

    # 4. Connect Nodes
    # Upstream -> Filter
    graph.add_edge(upstream_node, "Out", filter_node, "In")
    # Filter -> Transform
    graph.add_edge(filter_node, "Out", xform_node, "In")
    # Transform -> Spawner
    graph.add_edge(xform_node, "Out", spawner_node, "In")

    print("successfully added Ruins Layer!")

add_ruins_layer()
