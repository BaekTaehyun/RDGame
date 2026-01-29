import unreal

def fix_tree_randomness():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Fixing Tree Randomness in {graph_path} ---")
    graph = unreal.load_asset(graph_path)
    if not graph: return

    # Target Node: "TransformPoints_1" (Identified in early analysis as the Tree Transform)
    # Or find by name
    target_node = None
    for n in graph.nodes:
        if n.get_name() == "TransformPoints_1":
            target_node = n
            break
    
    if not target_node:
        print("TransformPoints_1 not found!")
        return
        
    settings = target_node.get_settings()
    if not settings: return

    print(f"Applying settings to {target_node.get_name()}...")

    # 1. Force Random Rotation
    settings.set_editor_property("ApplyRandomRotation", True)
    # Note: Rotator properties are structs.
    # We set 'RandomRotationMin' and 'Max'.
    # In Python, we can't easily modify struct members in-place if they are not exposed as object wrappers.
    # But set_editor_property expects a Rotator.
    
    settings.set_editor_property("RandomRotationMin", unreal.Rotator(0, 0, 0)) # Roll, Pitch, Yaw(Z)
    settings.set_editor_property("RandomRotationMax", unreal.Rotator(0, 360, 0))

    # 2. Force Uniform Scale
    # settings.set_editor_property("ApplyUniformScale", True) # Check property name
    # Scale min/max are Vectors usually.
    settings.set_editor_property("UniformScaleMin", 0.7)
    settings.set_editor_property("UniformScaleMax", 1.4)
    
    # 3. Apply Offset (Grid Breaker) if not present
    # settings.set_editor_property("OffsetMin", unreal.Vector(-50, -50, 0))
    # settings.set_editor_property("OffsetMax", unreal.Vector(50, 50, 0))

    print("Tree Randomness Applied (0-360 Rotation, 0.7-1.4 Scale).")

fix_tree_randomness()
