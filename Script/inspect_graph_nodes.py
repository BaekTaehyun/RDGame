import unreal

def inspect_nodes():
    factory = unreal.PCGGraphFactory()
    # Create temp graph
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset("Temp_Node_Inspect", "/Game", unreal.PCGGraph, factory)
    
    print(f"Graph Created: {graph.get_path_name()}")
    print("--- Nodes ---")
    for node in graph.nodes:
        print(f"Name: '{node.get_name()}' | Title: '{node.get_node_title()}' | Class: {node.get_class().get_name()}")

    # Cleanup
    unreal.EditorAssetLibrary.delete_asset(graph.get_path_name())

inspect_nodes()
