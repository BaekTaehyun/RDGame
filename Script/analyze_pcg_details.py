import unreal

def analyze_details():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Detailed Analysis: {graph_path} ---")
    
    graph = unreal.load_asset(graph_path)
    if not graph:
        print("Graph not found.")
        return

    # 1. Build Property Map for Key Settings
    def get_node_status(node, settings):
        status = []
        if settings:
             # Check Enabled
             try:
                 if not settings.b_enabled: status.append("[DISABLED]")
             except: pass
             
             # Check Debug
             try:
                 if settings.debug: status.append("[DEBUG]")
             except: pass
        return " ".join(status)

    # 2. Iterate Nodes & Connections
    # PCGGraph doesn't always expose edges directly in a simple list in Python
    # We might need to look at 'node.get_output_pins()' or similar if edges aren't directly available.
    # However, standard PCG API usually has 'graph.get_all_connections()' or similar?
    # Let's try iterating nodes and assuming we can't easily get edges without a specific API check.
    # ACTUALLY: Inspecting previous logs, we didn't see edge properties on Node.
    
    # Alternative: Use 'input_pins' and 'output_pins' inspection if edges are missing.
    
    print(f"\n{'Node Name':<40} | {'Type':<30} | {'Status':<10} | {'Settings Summary'}")
    print("-" * 120)
    
    for node in graph.nodes:
        name = node.get_name()
        
        # Get Title
        title = name
        if hasattr(node, "node_title"):
            try: title = str(node.node_title)
            except: pass
            
        # Settings
        settings = node.get_settings()
        s_type = settings.get_class().get_name() if settings else "NoSettings"
        
        # Status
        status = get_node_status(node, settings)
        
        # Summary
        summary = ""
        if settings:
            if "StaticMeshSpawner" in s_type:
                # Count meshes
                try:
                    s = settings.mesh_selector_parameters
                    entries = s.get_editor_property("MeshEntries")
                    summary = f"Meshes: {len(entries)}"
                except:
                    summary = "Meshes: ?"
            elif "Filter" in s_type:
                try:
                    attr = settings.get_editor_property("TargetAttribute") # Verify property name?
                    summary = f"Filter: {attr}"
                except:
                    pass
            elif "Pruning" in s_type:
                try:
                    method = settings.get_editor_property("PruningType")
                    summary = f"Type: {method}"
                except:
                    pass

        print(f"{title:<40} | {s_type:<30} | {status:<10} | {summary}")
        
        # Connections (If available via property inspection)
        # Note: In Python, Edge inspection is tricky without verified API. 
        # We will try to inspect 'Pins' if edges fail.
        
    print("\n--- Connection Analysis (Hypothetical) ---")
    # Since we can't easily walk graph edges in Python without 'get_edges()',
    # we'll check if there's a workaround property.
    # (Leaving this part minimal to avoid crash, focusing on Node/Settings first)

analyze_details()
