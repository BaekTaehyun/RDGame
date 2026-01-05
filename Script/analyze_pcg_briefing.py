import unreal
import json

def get_enum_string(enum_val):
    # Helper to convert generic enum to string if possible, or stringify
    return str(enum_val).split('.')[-1] if '.' in str(enum_val) else str(enum_val)

def analyze_briefing():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"============================================================")
    print(f" PCG GRAPH BRIEFING: {graph_path}")
    print(f"============================================================")
    
    # 1. Get Topology from C++
    if not hasattr(unreal, "DungeonAssetUtils"):
        print("[Error] C++ Utils not found.")
        return
        
    topo_json = unreal.DungeonAssetUtils.analyze_pcg_topology(graph_path)
    try:
        topo_data = json.loads(topo_json)
    except:
        print("[Error] Invalid Topology JSON")
        return
        
    topo_map = {n["Name"]: n.get("Outbound", []) for n in topo_data.get("Nodes", [])}
    
    # 2. Load Graph for Property Inspection
    graph = unreal.load_asset(graph_path)
    nodes_by_name = {n.get_name(): n for n in graph.nodes}
    
    # 3. Analyze Each Node
    # Sort for logical flow (Source first if possible, or alphabetical)
    # Let's try to group by functionality
    
    print(f"{'Node (Connectivity)':<40} | {'Key Parameters'}")
    print("-" * 100)
    
    sorted_names = sorted(topo_map.keys())
    
    for name in sorted_names:
        outbound = topo_map.get(name, [])
        out_str = f"-> [{', '.join(outbound)}]" if outbound else "-> (End)"
        
        conn_str = f"{name} {out_str}"
        
        # Get Settings
        node = nodes_by_name.get(name)
        params = []
        
        if node:
            settings = node.get_settings()
            if settings:
                s_class = settings.get_class().get_name()
                
                # Attribute Filter
                if "Filtering" in s_class:
                    try:
                        # Attempt to read TargetAttribute or InputSource
                        # Note: Structs formatted as strings usually
                        # Logic: Operator, Threshold
                        op = "?"
                        try: op = get_enum_string(settings.get_editor_property("Operator"))
                        except: pass
                        
                        # Target Attribute is often inside a struct InputSource
                        input_src = "?"
                        try: input_src = str(settings.get_editor_property("TargetAttribute").get_editor_property("Name"))
                        except: 
                            try: input_src = str(settings.get_editor_property("InputSource").get_editor_property("PropertyName"))
                            except: pass
                            
                        # Threshold types vary (int, float, etc) - usually not easily generic
                        # But we can list known ones
                        params.append(f"Filter: {input_src}")
                        params.append(f"Op: {op}")
                    except: pass
                    
                # Spawner
                elif "StaticMeshSpawner" in s_class:
                    try:
                        sel = None
                        if hasattr(settings, "mesh_selector_parameters"): sel = settings.mesh_selector_parameters
                        elif hasattr(settings, "mesh_selector_instance"): sel = settings.mesh_selector_instance
                        
                        if sel:
                            entries = sel.get_editor_property("MeshEntries")
                            params.append(f"Meshes: {len(entries)}")
                            # Check Weights
                            weights = []
                            for e in entries:
                                try: weights.append(str(e.get_editor_property("Weight")))
                                except: pass
                            if weights: params.append(f"Weights: {','.join(weights)}")
                    except: pass
                    
                # Distance
                elif "Distance" in s_class:
                    try:
                        dist_attr = "?"
                        try: dist_attr = str(settings.get_editor_property("TargetAttribute").get_editor_property("Name"))
                        except: pass
                        src_attr = "?"
                        try: src_attr = str(settings.get_editor_property("SourceAttribute").get_editor_property("Name"))
                        except: pass
                        params.append(f"Dist: {src_attr} to {dist_attr}")
                    except: pass
                    
                # Transform
                elif "Transform" in s_class:
                    try:
                        # Check if Absolute/Relative
                        params.append("Apply Transform")
                    except: pass
                    
                # Pruning
                elif "Pruning" in s_class:
                    try:
                        p_type = get_enum_string(settings.get_editor_property("PruningType"))
                        params.append(f"Type: {p_type}")
                    except: pass
                    
                # Debug Flag
                try: 
                    if settings.debug: params.append("[DEBUG ON]")
                except: pass
                
        param_str = ", ".join(params)
        print(f"{conn_str:<40} | {param_str}")

analyze_briefing()
