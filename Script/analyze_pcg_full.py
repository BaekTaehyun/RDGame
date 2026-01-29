import unreal

def analyze_full():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"============================================================")
    print(f" PCG GRAPH ANALYSIS REPORT")
    print(f" Target: {graph_path}")
    print(f"============================================================")
    
    graph = unreal.load_asset(graph_path)
    if not graph:
        print(" [Error] Graph asset not found.")
        return

    nodes = graph.nodes
    print(f" Total Nodes: {len(nodes)}\n")
    
    print(f"{'Node Title':<35} | {'Type':<25} | {'Conn':<6} | {'Status':<25} | {'Issues'}")
    print("-" * 120)
    
    issues_found = 0
    
    for node in nodes:
        # 1. Basic Info
        node_name = node.get_name()
        node_title = node_name
        if hasattr(node, "node_title"):
            try: node_title = str(node.node_title)
            except: pass
            
        # 2. Settings & Type
        settings = node.get_settings()
        type_name = "Node (No Settings)"
        status_flags = []
        node_issues = []
        
        if settings:
            type_name = settings.get_class().get_name().replace("PCG", "").replace("Settings", "")
            # Check Enabled
            try:
                if not settings.b_enabled: status_flags.append("DISABLED")
            except: pass
            # Check Debug
            try:
                if settings.debug: status_flags.append("DEBUG")
            except: pass
        else:
            if "Input" in node_name: type_name = "Input"
            elif "Output" in node_name: type_name = "Output"
            
        # 3. Connectivity (Edge Counts)
        in_edges = 0
        if hasattr(node, "input_pins"):
            for p in node.input_pins:
                if hasattr(p, "edges"): in_edges += len(p.edges)
                
        out_edges = 0
        if hasattr(node, "output_pins"):
            for p in node.output_pins:
                if hasattr(p, "edges"): out_edges += len(p.edges)
                
        # Derive Connectivity Status
        conn_str = f"{in_edges}/{out_edges}"
        
        # Check specific connectivity issues
        if type_name != "Input" and in_edges == 0:
            node_issues.append("Disconnected Input")
        if type_name != "Output" and out_edges == 0:
            # Not always an error, but worth noting for intermediate nodes
            # node_issues.append("Dead End") 
            pass

        # 4. Deep Inspection (Spawners)
        if settings and "StaticMeshSpawner" in type_name:
             try:
                 selector = None
                 if hasattr(settings, "mesh_selector_parameters"): selector = settings.mesh_selector_parameters
                 elif hasattr(settings, "mesh_selector_instance"): selector = settings.mesh_selector_instance
                 
                 if selector:
                     entries = selector.get_editor_property("MeshEntries")
                     if not entries or len(entries) == 0:
                         node_issues.append("No Mesh Entries")
                     else:
                         count = len(entries)
                         status_flags.append(f"{count} Meshes")
                         # Validate first few
                         for idx, entry in enumerate(entries):
                             try:
                                 desc = entry.get_editor_property("Descriptor")
                                 mesh = None
                                 try: mesh = desc.get_editor_property("StaticMesh")
                                 except: 
                                     try: mesh = desc.get_editor_property("Mesh")
                                     except: pass
                                 if not mesh:
                                     node_issues.append(f"Entry {idx} Missing Mesh")
                             except: pass
             except: pass
             
        # Format Output
        status_str = ", ".join(status_flags)
        issue_str = ", ".join(node_issues)
        
        if len(node_issues) > 0:
            issues_found += 1
            issue_str = f"<!> {issue_str}"
            
        print(f"{node_title[:35]:<35} | {type_name[:25]:<25} | {conn_str:<6} | {status_str:<25} | {issue_str}")

    print("-" * 120)
    print(f"Analysis Complete. Found {issues_found} nodes with potential issues.")

analyze_full()
