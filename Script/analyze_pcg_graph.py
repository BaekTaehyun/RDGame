import unreal

def analyze_graph(graph_path):
    print(f"--- Analyzing PCG Graph: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph or not isinstance(graph, unreal.PCGGraph):
        print(f"[Error] Could not load PCGGraph at {graph_path}")
        return

    nodes = graph.nodes
    print(f"Total Nodes: {len(nodes)}")
    
    # Statistics
    node_types = {}
    issues = []
    
    for node in nodes:
        node_name = node.get_name()
        node_title = node_name
        if hasattr(node, "node_title"):
            try:
                node_title = str(node.node_title)
            except:
                pass
        
        # Get Settings & Determine Type
        settings = node.get_settings()
        n_type = "PCGNode (No Settings)"
        
        if settings:
            n_type = settings.get_class().get_name()
        else:
            # Check for Input/Output by name or class if possible
            if "Input" in node_name: n_type = "PCGInputNode"
            elif "Output" in node_name: n_type = "PCGOutputNode"
            
        node_types[n_type] = node_types.get(n_type, 0) + 1
        
        # Check 1: Settings Existence
        if not settings and "Input" not in n_type and "Output" not in n_type: 
             issues.append(f"Node '{node_title}' ({node_name}) has NO Settings object.")
             
        # Check 2: Spawner Issues
        # Look for 'Spawner' in the settings class name
        if settings and "Spawner" in n_type:
             # Check Mesh Entries
             try:
                 # Check for Empty Entries in StaticMeshSpawner
                 if "StaticMeshSpawner" in n_type:
                     selector = None
                     if hasattr(settings, "mesh_selector_parameters"): selector = settings.mesh_selector_parameters
                     elif hasattr(settings, "mesh_selector_instance"): selector = settings.mesh_selector_instance
                     
                     if selector:
                         entries = selector.get_editor_property("MeshEntries")
                         if not entries or len(entries) == 0:
                             issues.append(f"[Warning] Spawner '{node_title}' has NO Mesh Entries.")
                         else:
                             # Check for valid meshes
                             for idx, entry in enumerate(entries):
                                 try:
                                     # Entry -> Descriptor -> StaticMesh
                                     desc = entry.get_editor_property("Descriptor")
                                     
                                     # Try StaticMesh or Mesh
                                     mesh = None
                                     try: mesh = desc.get_editor_property("StaticMesh")
                                     except: 
                                         try: mesh = desc.get_editor_property("Mesh")
                                         except: pass
                                         
                                     if not mesh:
                                          issues.append(f"[Error] Spawner '{node_title}' Entry #{idx} has NO Static Mesh allocated.")
                                 except Exception as e_entry:
                                     pass
             except Exception as e:
                 print(f"Error inspecting spawner {node_name}: {e}")

    # Report
    print("\n--- Node Composition ---")
    for k, v in node_types.items():
        print(f"  {k}: {v}")
        
    print("\n--- Potential Issues ---")
    if len(issues) == 0:
        print("  None detected.")
    else:
        for i in issues:
            print(f"  [Warning] {i}")
            
analyze_graph("/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood")
