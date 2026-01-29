import unreal
import sys
import pcg_visual_tools
import imp
imp.reload(pcg_visual_tools)

def run_v8():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print("--- Running V8 PCG Fix (Robust Search) ---")
    
    tools = pcg_visual_tools.PCGVisualTools(graph_path)
    
    # 1. Density Filter (Search "CopyPoints" or "Distance")
    # Using "CopyPoints" (no space) helps class check, "Copy Points" (space) helps Title check.
    # Our improved find_node handles both.
    
    existing_filter = tools.find_node("Tree_Reduction_Filter")
    if existing_filter:
        print("Updated existing Density Filter.")
        s = existing_filter.get_settings()
        s.set_editor_property("LowerBound", 0.6) 
        s.set_editor_property("UpperBound", 1.0)
    else:
        # Inject between CopyPoints and Distance
        # Note: If multiple CopyPoints exist, this picks the first.
        # Screenshot shows only one main chain for trees.
        
        df = unreal.PCGDensityFilterSettings()
        df.set_editor_property("LowerBound", 0.6) # Kill 60%
        df.set_editor_property("UpperBound", 1.0)
        
        node = tools.inject_node("Copy Points", "Distance", df, "Tree_Reduction_Filter")
        if node: print("Injected Density Filter (60% Reduction).")
        else: print("Failed to inject density filter (Nodes not found).")

    # 2. Jitter
    # Search for "TransformPoints"
    tools.apply_transform("Transform Points", offset=80, rotation=True, scale_min=0.6, scale_max=1.7)

    # 3. Ruins (Re-verify)
    ruins_filter = tools.find_node("Ruins_Filter")
    if ruins_filter:
        r_var = tools.find_node("Ruins_Variator")
        if r_var:
            vs = r_var.get_settings()
            vs.offset_min = unreal.Vector(0,0,50)
            vs.offset_max = unreal.Vector(0,0,100)
            vs.scale_min = unreal.Vector(2.5, 2.5, 2.5) # Force Huge
            vs.scale_max = unreal.Vector(3.5, 3.5, 3.5)
            print("Verified Ruins Settings.")
    
    tools.save()
    print("V8 Complete.")

run_v8()
