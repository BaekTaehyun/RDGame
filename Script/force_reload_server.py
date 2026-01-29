import unreal
import unreal_socket_server
import imp
import time

# 1. Stop Server
print("--- Reloading Unreal MCP Server ---")
try:
    unreal_socket_server.stop_server()
except:
    pass

# 2. Reload Module
imp.reload(unreal_socket_server)

# 3. Start Server
unreal_socket_server.start_server()
print("--- Server Reloaded & Restarted ---")
