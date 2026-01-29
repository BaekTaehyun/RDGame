import unreal

def check_utils():
    print("--- Checking DungeonAssetUtils ---")
    
    # 1. Check Class Existence
    if not hasattr(unreal, "DungeonAssetUtils"):
        print("[Error] Class 'unreal.DungeonAssetUtils' not found.")
        print("  Possible causes: Module not loaded, Compilation failed, or Typo.")
        return
        
    cls = unreal.DungeonAssetUtils
    print(f"Class Found: {cls}")
    
    # 2. Check Methods
    print("--- Available Attributes ---")
    found = False
    for d in dir(cls):
        if "pcg" in d.lower() or "topo" in d.lower():
            print(f"  [MATCH] {d}")
            found = True
            
    if not found:
        print("  [FAIL] analyze_pcg_topology not found in directory.")
        print("  Did you Compile/Live Coding AFTER the code fix?")
    else:
        print("  [SUCCESS] Function appears to be present.")

check_utils()
