# UE4 TCP Socket Plugin
Tcp Socket Plugin facilitates communication with a TCP server purely in blueprints.

# List of Features
- Multiple connections
- Multi-threading: each connection runs on its own thread
- Detects disconnects as soon as they happen
- Even dispatchers: OnConnected, OnDisconnected (also triggers when connection fails), OnMessageReceived
- Serialize and deserialize basic types: Uint8, Int32, Float, String
- Free and open source under MIT license

# Usage
Create a blueprint actor inheriting from TcpSocketConnection, drop it into level and use these nodes:
![Alt text](/functionality.jpg?raw=true "Functionality")

# Platforms
Intended for all platforms that support sockets and multithreading, which is most of them, except HTML5. <br />
Tested on platforms: Windows
