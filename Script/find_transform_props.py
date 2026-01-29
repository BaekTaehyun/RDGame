import unreal

def find_translation_props():
    print("--- Searching for Translation/Offset Properties ---")
    settings = unreal.PCGTransformPointsSettings()
    
    candidates = ["offset", "trans", "position", "loc"]
    
    for d in dir(settings):
        d_lower = d.lower()
        if any(c in d_lower for c in candidates):
            print(f"Candidate: {d}")

find_translation_props()
