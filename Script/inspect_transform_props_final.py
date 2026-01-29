import unreal
import inspect

def inspect_props():
    print("--- Inspecting PCGTransformPointsSettings ---")
    obj = unreal.PCGTransformPointsSettings()
    
    # Method 1: Dir
    print("Py Wrapper Dir:")
    for d in dir(obj):
        if not d.startswith("_"): print(f"  {d}")

    # Method 2: Reflection via Class (Property names)
    print("\nUnreal Class Properties:")
    uclass = obj.get_class()
    for p in unreal.UnrealReflection.get_class_properties(uclass):
        print(f"  {p}")

inspect_props()
