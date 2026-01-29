import unreal

class PCGVisualTools:
    def __init__(self, graph_path):
        self.graph_path = graph_path
        self.graph = unreal.load_asset(graph_path)
        if not self.graph:
            raise Exception(f"Graph not found: {graph_path}")

    def find_node(self, name_fuzzy):
        name_lower = name_fuzzy.lower().replace(" ", "") # Remove spaces for class match
        for n in self.graph.nodes:
            # 1. Check Name
            if name_fuzzy.lower() in n.get_name().lower(): return n
            
            # 2. Check Title
            try:
                title = str(n.get_editor_property("NodeTitle"))
                if name_fuzzy.lower() in title.lower(): return n
            except: pass

            # 3. Check Settings Class (Robust)
            # e.g. "CopyPoints" in "PCGCopyPointsSettings"
            s = n.get_settings()
            if s:
                cls_name = s.get_class().get_name().lower()
                if name_lower in cls_name: return n
                
        return None

    def get_upstream_node(self, node, pin_label="In"):
        # Find what connects to this node's input
        for pin in node.get_input_pins():
            if pin.get_editor_property("Label") == pin_label:
                if len(pin.edges) > 0:
                    # Edge: UpstreamPin -> DownstreamPin
                    # We want the Upstream Node
                    # 5.3 Edge structure might be difficult to traverse backwards via Python
                    # Usually: edge.upstream_pin.node
                    pass
        return None  # Complexity: Edges traversal in Py wrapper is inconsistent

    def inject_node(self, upstream_node_name, downstream_node_name, new_node_settings, new_node_title):
        """
        Injects a new node between two existing nodes.
        Reference: CopyPoints -> Distance
        """
        up = self.find_node(upstream_node_name)
        down = self.find_node(downstream_node_name)
        
        if not up or not down:
            print(f"Error: Could not find {upstream_node_name} or {downstream_node_name}")
            return None

        # Safe Title Logging
        up_name = up.get_editor_property('NodeTitle') if up else "None"
        if not str(up_name) or str(up_name) == "None": up_name = up.get_name()
        
        down_name = down.get_editor_property('NodeTitle') if down else "None"
        if not str(down_name) or str(down_name) == "None": down_name = down.get_name()

        print(f"Injecting {new_node_title} between {up_name} and {down_name}")
        
        # 1. Add New Node
        new_node = self.graph.add_node_instance(new_node_settings)
        new_node.node_title = new_node_title
        
        # Position: Skip (PCGNode in 5.3 doesn't expose position_x easily via python)
        # We rely on manual layout later or auto-layout if available.

        # 2. Break Old Connection
        # We assume 'Out' -> 'In' standard.
        self.graph.remove_edge(up, "Out", down, "In")
        self.graph.remove_edge(up, "Out", down, "Source") # CopyPoints might use Source/Target pins
        
        # 3. Connect New Path
        self.graph.add_edge(up, "Out", new_node, "In")
        self.graph.add_edge(new_node, "Out", down, "In") # Or 'Source'
        
        print("Injection Complete")
        return new_node

    def apply_transform(self, node_name, offset=0, rotation=False, scale_min=1.0, scale_max=1.0):
        node = self.find_node(node_name)
        if not node:
            print(f"Node {node_name} not found")
            return

        s = node.get_settings()
        # Verified Attributes
        if offset > 0:
            s.offset_min = unreal.Vector(-offset, -offset, 0)
            s.offset_max = unreal.Vector(offset, offset, 0)
        
        if rotation:
            s.rotation_max = unreal.Rotator(0, 360, 0)
            
        s.scale_min = unreal.Vector(scale_min, scale_min, scale_min)
        s.scale_max = unreal.Vector(scale_max, scale_max, scale_max)
        
        # Force Booleans (Reflection fallback)
        try: s.set_editor_property("bApplyNodeSpecificOffset", False) 
        except: pass
        try: s.set_editor_property("bApplyRandomRotation", True) 
        except: pass
        try: s.set_editor_property("bApplyUniformScale", True)
        except: pass
        
        print(f"Applied Transform to {node_name}: Off={offset}, Rot={rotation}, Scale={scale_min}~{scale_max}")

    def save(self):
        unreal.EditorAssetLibrary.save_loaded_asset(self.graph)
