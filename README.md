# UE4 TCP Socket Plugin
Tcp Socket Plugin facilitates communication with a TCP server purely in blueprints. Client-only functionality.

# List of Features
- Multiple connections
- Multi-threading: each connection runs on its own thread
- Detects disconnects as soon as they happen
- Event dispatchers: OnConnected, OnDisconnected (also triggers when connection fails), OnMessageReceived
- Serialize and deserialize basic types: Uint8, Int32, Float, String
- Free and open source under MIT license

# Usage in Blueprints
Create a blueprint actor inheriting from TcpSocketConnection, drop it into level and use these nodes:
![Alt text](/functionality.jpg?raw=true "Functionality")

# Usage in C++
This is only an example.
In .h
```cpp
UCLASS()
class EXAMPLE_API ACppSocketConnection : public ATcpSocketConnection
{
	GENERATED_BODY()
 public:
	UFUNCTION()
	void OnConnected(int32 ConnectionId);

	UFUNCTION()
	void OnDisconnected(int32 ConId);

	UFUNCTION()
	void OnMessageReceived(int32 ConId, TArray<uint8>& Message);
  
  	UFUNCTION(BlueprintCallable)
	void ConnectToGameServer();
	
	UPROPERTY()
	int32 connectionIdGameServer;
}
```

In .cpp
```cpp
void ACppSocketConnection::ConnectToGameServer() {
	if (isConnected(connectionIdGameServer))
	{
		UE_LOG(LogError, Log, TEXT("Log: Can't connect SECOND time. We're already connected!"));
		return;
	}
	FTcpSocketDisconnectDelegate disconnectDelegate;
	disconnectDelegate.BindDynamic(this, &ACppSocketConnection::OnDisconnected);
	FTcpSocketConnectDelegate connectDelegate;
	connectDelegate.BindDynamic(this, &ACppSocketConnection::OnConnected);
	FTcpSocketReceivedMessageDelegate receivedDelegate;
	receivedDelegate.BindDynamic(this, &ACppSocketConnection::OnMessageReceived);
	Connect("127.0.0.1", 3500, disconnectDelegate, connectDelegate, receivedDelegate, connectionIdGameServer);
}

void ACppSocketConnection::OnConnected(int32 ConId) {
	UE_LOG(LogTemp, Log, TEXT("Log: Connected to server."));
}

void ACppSocketConnection::OnDisconnected(int32 ConId) {
	UE_LOG(LogTemp, Log, TEXT("Log: OnDisconnected."));
}

void ACppSocketConnection::OnMessageReceived(int32 ConId, TArray<uint8>& Message) {
	UE_LOG(LogTemp, Log, TEXT("Log: Received message."));
  	// In this example, we always encode messages a certain way:
  	// The first 4 bytes contain the length of the rest of the message.
  	while (Message.Num() != 0) {
		// read expected length
		int32 msgLength = Message_ReadInt(Message);
		if (msgLength == -1) // when we can't read 4 bytes, the method returns -1
			return;
		TArray<uint8> yourMessage;
		// read the message itself
		if (!Message_ReadBytes(msgLength, Message, yourMessage)) {
			// If it couldn't read expected number of bytes, something went wrong.
			// Print a UE_LOG here, while your project is in development.
			continue;
		}
		// If the message was read, then treat "yourMessage" here!
		// ...
		// And then we go back to the "while", because we may have received multiple messages in a frame, 
		// so they all have to be read.
  	}
}
```

# Platforms
Intended for all platforms that support sockets and multithreading, which is most of them, except HTML5. <br />
Tested on platforms: Windows
