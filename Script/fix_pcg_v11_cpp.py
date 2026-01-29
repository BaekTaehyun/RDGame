import unreal
import time

def run_v11_cpp_connect():
    # Wait strictly for C++ hot reload if needed, but user must trigger compile.
    # We Assume compilation is done.
    
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- [V11] C++ Connection Fix: {graph_path} ---")

    # Connect Ruins Chain
    # 1. SelfPruning -> Ruins_Filter
    print("Linking SelfPruning -> Ruins_Filter...")
    res1 = unreal.DungeonAssetUtils.connect_pcg_nodes(graph_path, "SelfPruning", "Ruins_Filter", "Out", "In")
    print(f"Result: {res1}")

    # 2. Ruins_Filter -> Ruins_Variator
    print("Linking Ruins_Filter -> Ruins_Variator...")
    res2 = unreal.DungeonAssetUtils.connect_pcg_nodes(graph_path, "Ruins_Filter", "Ruins_Variator", "Out", "In")
    print(f"Result: {res2}")

    # 3. Ruins_Variator -> Spawner_Ruins
    print("Linking Ruins_Variator -> Spawner_Ruins...")
    res3 = unreal.DungeonAssetUtils.connect_pcg_nodes(graph_path, "Ruins_Variator", "Spawner_Ruins", "Out", "In")
    print(f"Result: {res3}")

    if res1 and res2 and res3:
        print("\n*** SUCCESS: All Ruins Nodes Connected via C++ ***")
    else:
        print("\n*** WARNING: Some connections failed. Check Output Log for 'ConnectPCGNodes' errors ***")

run_v11_cpp_connect()
