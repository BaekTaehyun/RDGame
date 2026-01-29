import unreal

def inspect_graph_state():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Inspecting {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph: return
    
    for n in graph.nodes:
        # Get Title (if overridden) or Name
        title = n.get_editor_property("NodeTitle")
        name = n.get_name()
        print(f"[{name}] Title='{title}' Class={n.get_settings().get_class().get_name()}")
        
        # Check for Grid settings if this is the grid node
        if "CreatePointsGrid" in n.get_settings().get_class().get_name():
             s = n.get_settings()
             # CellSize is usually a property 'CellSize' (Vector) or similar
             try:
                 # It might be in 'GridParameters' or direct
                 # 5.3: 'CellSize' vector
                 cs = s.get_editor_property("CellSize")
                 print(f"   -> Grid CellSize: {cs}")
             except:
                 pass

inspect_graph_state()
