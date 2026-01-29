import unreal

def inspect_direct():
    print(">>> Inspecting Descriptor Directly")
    
    inst = unreal.PCGMeshSelectorWeightedEntry()
    
    try:
        # Try direct generic access
        desc = inst.get_editor_property("Descriptor")
        print(f"[O] Found Descriptor: {desc} (Type: {type(desc)})")
        
        print("--- Descriptor Attributes ---")
        for d in dir(desc):
            if not d.startswith("_"):
                print(f"  {d}")
                
        # Check standard mesh properties
        if hasattr(desc, "static_mesh"):
            print("[!] Found 'static_mesh' attribute")
        elif hasattr(desc, "mesh"):
            print("[!] Found 'mesh' attribute")
            
    except Exception as e:
        print(f"[X] Failed to access Descriptor: {e}")
        
inspect_direct()
