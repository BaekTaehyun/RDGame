import unreal

def inspect_transform():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    node_name = "TransformPoints_1"
    
    print(f"--- Inspecting {node_name} ---")
    graph = unreal.load_asset(graph_path)
    if not graph: return
    
    target_node = None
    for n in graph.nodes:
        if n.get_name() == node_name:
            target_node = n
            break
            
    if target_node:
        settings = target_node.get_settings()
        
        # Properties to check:
        # ApplyRandomRotation, RandomRotationMin/Max
        # ApplyRandomScale, UniformScale, RandomScaleMin/Max
        
        props = [
            "ApplyRandomRotation", "RandomRotationMin", "RandomRotationMax",
            "ApplyRandomScale", "UniformScale", "RandomScaleMin", "RandomScaleMax"
        ]
        
        for p in props:
            try:
                val = settings.get_editor_property(p)
                print(f"{p}: {val}")
            except:
                print(f"{p}: [ATTR NOT FOUND]")

inspect_transform()
