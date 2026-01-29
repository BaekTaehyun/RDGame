import unreal

def debug_node_api():
    print("--- Debugging PCG Node API ---")
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    graph = unreal.load_asset(graph_path)
    if not graph: return

    # Pick a few representative nodes
    target_nodes = ["CreatePointsGrid", "SelfPruning", "Ruins_Filter", "Tree_Reduction_Filter"]
    
    for n in graph.nodes:
        name = n.get_name()
        title = str(n.get_editor_property("NodeTitle"))
        
        matches = False
        for t in target_nodes:
            if t in name or t in title: matches = True
        
        if matches:
            print(f"\nNode: {name} ({title})")
            print("  [DIR(Node)]")
            # Limit dir output to non-underscored
            for d in dir(n):
                if not d.startswith("_"): print(f"    {d}")
                
            # Check Settings for Pin Labels?
            s = n.get_settings()
            if s:
                print(f"  [Settings Class]: {s.get_class().get_name()}")
                # Does settings have pin info?
                print("  [DIR(Settings)]")
                for d in dir(s):
                    if "pin" in d.lower() or "label" in d.lower():
                        print(f"    {d}")

debug_node_api()
