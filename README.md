# UE4TcpSocketPlugin
Tcp Socket Plugin facilitates communication with a TCP server purely in blueprints.

# Usage
Create a blueprint actor inheriting from TcpSocketConnection, drop it into level and use it.

# List of Features
- Multiple connections
- Multi-threading: each connection runs on its own thread
- Detects disconnects as soon as they happen
- Even dispatchers: OnConnected, OnDisconnected (also triggers when connection fails), OnMessageReceived
- Serialize and deserialize basic types: Uint8, Int32, Float, String
- Free and open source under MIT license

# Platforms
Intended for all platforms that support sockets and multithreading, which is most of them, except HTML5.
Tested on platforms: Windows
