import sys
# Script Path
sys.path.append(r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script")

# Import and Start
import unreal_mcp_server
import importlib
importlib.reload(unreal_mcp_server)
unreal_mcp_server.start_server()

print("MCP Server Started on Port 1000")
