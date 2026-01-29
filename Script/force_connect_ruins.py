import unreal

def force_connect_ruins():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Force Connecting Ruins Chain: {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph: return

    def find_node(name_fuzzy):
        name_clean = name_fuzzy.lower().replace(" ", "")
        for n in graph.nodes:
            if name_fuzzy.lower() in n.get_name().lower(): return n
            try:
                if fuzzy_name.lower() in str(n.get_editor_property("NodeTitle")).lower(): return n
            except: pass
            s = n.get_settings()
            if s and name_clean in s.get_class().get_name().lower(): return n
        return None

    # Implement find helper correctly (copy paste error fix above)
    def find_node_robust(name):
        clean = name.lower().replace(" ", "")
        for n in graph.nodes:
            if name.lower() in n.get_name().lower(): return n
            if clean in n.get_name().lower(): return n
            try:
                t = str(n.get_editor_property("NodeTitle"))
                if name.lower() in t.lower(): return n
            except: pass
        return None

    # Get Nodes
    up = find_node_robust("SelfPruning")
    rf = find_node_robust("Ruins_Filter")
    rv = find_node_robust("Ruins_Variator")
    rs = find_node_robust("Spawner_Ruins")

    if up and rf and rv and rs:
        print("All Nodes Found. Linking...")
        try:
            # Add Edge (Upstream, Label, Downstream, Label)
            # Default for most is Out -> In
            graph.add_edge(up, "Out", rf, "In")
            print(" - Linked SelfPruning -> Ruins_Filter")
            
            graph.add_edge(rf, "Out", rv, "In")
            print(" - Linked Ruins_Filter -> Ruins_Variator")
            
            graph.add_edge(rv, "Out", rs, "In")
            print(" - Linked Ruins_Variator -> Spawner_Ruins")
            
            unreal.EditorAssetLibrary.save_loaded_asset(graph)
            print("SUCCESS: Connections enforced and Graph Saved.")
        except Exception as e:
            print(f"Error linking: {e}")
    else:
        print("CRITICAL: Some nodes missing despite V10 PASS?")
        print(f"SelfPruning: {up}")
        print(f"Ruins_Filter: {rf}")
        print(f"Ruins_Variator: {rv}")
        print(f"Spawner_Ruins: {rs}")

force_connect_ruins()
