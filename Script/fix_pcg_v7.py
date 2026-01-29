import unreal
import sys
import os

# Ensure script dir is in path
sys.path.append(r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script")
import pcg_visual_tools
import imp
imp.reload(pcg_visual_tools) # Force reload to get latest changes

def run_v7():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print("--- Running V7 PCG Enhancement (Modular Tools) ---")
    
    try:
        tools = pcg_visual_tools.PCGVisualTools(graph_path)
        
        # 1. Inject Density Filter (To fix blockiness)
        # We need to find 'CopyPoints' and 'Distance'
        # If 'Tree_Density_Filter' already exists, update it.
        existing_filter = tools.find_node("Tree_Reduction_Filter")
        if existing_filter:
            print("Tree_Reduction_Filter exists. Updating settings.")
            s = existing_filter.get_settings()
            s.set_editor_property("LowerBound", 0.5) # Kill 50%
            s.set_editor_property("UpperBound", 1.0)
        else:
            # Create density filter settings
            df = unreal.PCGDensityFilterSettings()
            df.set_editor_property("LowerBound", 0.5) # Keep 0.5 -> 1.0 (50% density)
            df.set_editor_property("UpperBound", 1.0)
            
            # Use 'Distance' or 'BoundsModifier'?
            # Graph: CopyPoints -> Distance.
            # We want to reduce BEFORE distance calculation? Or After?
            # Reducing before is more efficient.
            tools.inject_node("Copy Points", "Distance", df, "Tree_Reduction_Filter")

        # 2. Apply Jitter (Using Verified Props)
        # Offset +/- 60, Scale 0.6-1.5
        tools.apply_transform("Transform Points", offset=60, rotation=True, scale_min=0.6, scale_max=1.5)

        # 3. Fix Ruins (Ensure Visibility)
        ruins_filter = tools.find_node("Ruins_Filter")
        if ruins_filter:
            # We don't recreate, just ensure the Variator has High Offset
            # But the Variator node might be unnamed in python if we didn't store it.
            # We can find it by proximity or name.
            r_var = tools.find_node("Ruins_Variator")
            if r_var:
                vs = r_var.get_settings()
                vs.offset_min = unreal.Vector(0,0,50) # Force Lift
                vs.offset_max = unreal.Vector(0,0,100)
                vs.scale_min = unreal.Vector(2.0, 2.0, 2.0)
                vs.scale_max = unreal.Vector(3.5, 3.5, 3.5)
                print("Ruins Visibility Enhanced (Lifted & Scaled)")

        tools.save()
        print("V7 Enhancement Applied Successfully.")

    except Exception as e:
        print(f"V7 Error: {e}")

run_v7()
