import unreal

def inspect_struct():
    print("--- Inspecting PCGMeshSelectorWeightedEntry ---")
    try:
        entry = unreal.PCGMeshSelectorWeightedEntry()
        print("Dir(entry):")
        for d in dir(entry):
            if not d.startswith("__"):
                val = getattr(entry, d)
                print(f"  {d}: {type(val)} = {val}")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    inspect_struct()
