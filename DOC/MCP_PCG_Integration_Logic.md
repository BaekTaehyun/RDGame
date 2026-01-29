# MCP PCG Integration Logic

This document details the technical implementation and API structure for the Unreal MCP Server's PCG capabilities.

## 1. System Architecture

The integration consists of two main components:
1.  **Unreal Socket Server (`unreal_socket_server.py`)**: Runs inside the Unreal Engine Python environment, listening on TCP port 3001. It handles direct Unreal API calls.
2.  **MCP Bridge (`unreal_mcp_bridge.py`)**: Runs as an external process (controlled by the LLM client), implementing the MCP protocol (stdio) and forwarding requests to the socket server.

## 2. PCG Mesh Assignment Logic

Attempts to set the `MeshEntries` property on a `PCGStaticMeshSpawnerSettings` node require special handling because `MeshEntries` is not a direct property of the settings object.

### The Problem
The `PCGStaticMeshSpawnerSettings` object uses a `MeshSelectorParameters` (or `Instance`) struct to hold the mesh list.
Chain: `Node` -> `Settings` -> `MeshSelectorParameters` -> `MeshEntries` (Array) -> `Entry` -> `Descriptor` -> `StaticMesh`

### The Solution (Smart Setter)
The `unreal_socket_server.py` implements a "smart" `set_smart_property` function that:
1.  Checks if the target object has the property directly.
2.  If not, checks if a `mesh_selector_parameters` (or `mesh_selector_instance`) exists.
3.  **Auto-Converts** a simple list of strings handling to the complex struct chain.

#### Input Format
```json
"MeshEntries": ["/Game/Path/To/Mesh.Mesh"]
```

#### Internal Logic
```python
if p_name == "MeshEntries" and isinstance(p_val, list):
    # 1. Instantiate PCGMeshSelectorWeightedEntry (Struct)
    # 2. Access 'Descriptor' property (PCGSoftISMComponentDescriptor)
    # 3. Set 'StaticMesh' or 'Mesh' property on the descriptor
    # 4. Set 'Weight' to 1
    # 5. Assign array back to selector.MeshEntries
```

## 3. Supported Commands (Socket Protocol)

### `create_pcg_graph`
*   **Request**: `{"command": "create_pcg_graph", "package_path": "...", "asset_name": "..."}`
*   **Logic**: Uses `AssetToolsHelpers` and `PCGGraphFactory` to create a new asset.

### `add_pcg_node`
*   **Request**: `{"command": "add_pcg_node", "graph_path": "...", "node_class": "...", "position_x": int, "position_y": int}`
*   **Logic**:
    *   Loads graph.
    *   Calls `graph.add_node_of_type(unreal.NodeClass)`.
    *    Calls `node.modify()` and `node.set_node_position(...)`.

### `connect_pcg_nodes`
*   **Request**: `{"command": "connect_pcg_nodes", "graph_path": "...", "upstream_node": "...", "downstream_node": "..."}`
*   **Logic**:
    *   Finds nodes by name (e.g., "Input", "StaticMeshSpawner_0").
    *   Calls `upstream_node.add_edge_to(out_pin, downstream_node, in_pin)`.

### `set_pcg_node_properties`
*   **Request**: `{"command": "set_pcg_node_properties", "graph_path": "...", "node_name": "...", "properties": {...}}`
*   **Logic**:
    *   Locates node and retrieves `Settings` object.
    *   Iterates through properties using the **Smart Setter** logic described above.

## 4. MCP Tool Definitions

The `unreal_mcp_bridge.py` exposes these socket commands as standard MCP tools:
*   `create_pcg_graph`
*   `add_pcg_node`
*   `connect_pcg_nodes`
*   `set_pcg_node_properties`

Refer to the `inputSchema` in the bridge script for exact JSON schemas.
