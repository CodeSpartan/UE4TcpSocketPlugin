// SpartanTools 2019

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Runnable.h"
#include "Containers/Queue.h"
#include "TcpSocketConnection.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FTcpSocketDisconnectDelegate, int32, ConnectionId);
DECLARE_DYNAMIC_DELEGATE_OneParam(FTcpSocketConnectDelegate, int32, ConnectionId);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FTcpSocketReceivedMessageDelegate, int32, ConnectionId, const TArray<uint8>&, Message);

UCLASS(Blueprintable, BlueprintType)
class TCPSOCKETPLUGIN_API ATcpSocketConnection : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATcpSocketConnection();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/* Returns the ID of the new connection. */
	UFUNCTION(BlueprintCallable, Category = "Socket")
	void Connect(const FString& ipAddress, int32 port, 
		const FTcpSocketDisconnectDelegate& OnDisconnected, const FTcpSocketConnectDelegate& OnConnected,
		const FTcpSocketReceivedMessageDelegate& OnMessageReceived, int32& ConnectionId);

	/* Disconnect from connection ID. */
	UFUNCTION(BlueprintCallable, Category = "Socket")
	void Disconnect(int32 ConnectionId);

	/* False means we're not connected to socket and the data wasn't sent. "True" doesn't guarantee that it was successfully sent, 
	only that we were still connected when we initiating the sending process. */
	UFUNCTION(BlueprintCallable, Category = "Socket") // use meta to set first default param to 0
	bool SendData(int32 ConnectionId, TArray<uint8> DataToSend);

	UFUNCTION(Category = "Socket")
	void ExecuteOnConnected(int32 WorkerId);

	UFUNCTION(Category = "Socket")
	void ExecuteOnDisconnected(int32 WorkerId);

	UFUNCTION(Category = "Socket")
	void ExecuteOnMessageReceived(int32 ConnectionId);

	/*UFUNCTION(BlueprintPure, meta = (DisplayName = "Append Bytes", CommutativeAssociativeBinaryOperator = "true"), Category = "Socket")
	static TArray<uint8> Concat_BytesBytes(const TArray<uint8>& A, const TArray<uint8>& B);*/

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Append Bytes", CommutativeAssociativeBinaryOperator = "true"), Category = "Socket")
	static TArray<uint8> Concat_BytesBytes(TArray<uint8> A, TArray<uint8> B);

	/** Converts an integer to an array of bytes */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Int To Bytes", CompactNodeTitle = "->", Keywords = "cast convert", BlueprintAutocast), Category = "Socket")
	static TArray<uint8> Conv_IntToBytes(int32 InInt);

	/** Converts a string to an array of bytes */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "String To Bytes", CompactNodeTitle = "->", Keywords = "cast convert", BlueprintAutocast), Category = "Socket")
	static TArray<uint8> Conv_StringToBytes(const FString& InStr);

	/** Converts a float to an array of bytes */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Float To Bytes", CompactNodeTitle = "->", Keywords = "cast convert", BlueprintAutocast), Category = "Socket")
	static TArray<uint8> Conv_FloatToBytes(float InFloat);

	/** Converts a byte to an array of bytes */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Byte To Bytes", CompactNodeTitle = "->", Keywords = "cast convert", BlueprintAutocast), Category = "Socket")
	static TArray<uint8> Conv_ByteToBytes(uint8 InByte);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Read Int", Keywords = "read int"), Category = "Socket")
	static int32 Message_ReadInt(UPARAM(ref) TArray<uint8>& Message);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Read Byte", Keywords = "read byte int8 uint8"), Category = "Socket")
	static uint8 Message_ReadByte(UPARAM(ref) TArray<uint8>& Message);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Read Float", Keywords = "read float"), Category = "Socket")
	static float Message_ReadFloat(UPARAM(ref) TArray<uint8>& Message);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Read String", Keywords = "read string"), Category = "Socket")
	static FString Message_ReadString(UPARAM(ref) TArray<uint8>& Message, int32 StringLength);

	/* Used by the separate threads to print to console on the main thread. */
	static void PrintToConsole(FString Str, bool Error);

	/* Buffer size in bytes. Currently not used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	int32 SendBufferSize = 16384;

	/* Buffer size in bytes. It's set only when creating a socket, never afterwards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	int32 ReceiveBufferSize = 16384;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	float TimeBetweenTicks = 0.015f;

private:
	TMap<int32, TSharedRef<class FTcpSocketWorker>> TcpWorkers;

	FTcpSocketDisconnectDelegate DisconnectedDelegate;
	FTcpSocketConnectDelegate ConnectedDelegate;
	FTcpSocketReceivedMessageDelegate MessageReceivedDelegate;
};

class FTcpSocketWorker : public FRunnable, public TSharedFromThis<FTcpSocketWorker>
{

	/** Thread to run the worker FRunnable on */
	FRunnableThread* Thread = nullptr;

private:
	class FSocket* Socket;
	FString ipAddress;
	int port;
	ATcpSocketConnection* ThePC;
	int32 id;
	int32 RecvBufferSize;
	int32 ActualRecvBufferSize;
	int32 SendBufferSize;
	int32 ActualSendBufferSize;
	float TimeBetweenTicks;
	bool bConnected = false;	

	TQueue<TArray<uint8>, EQueueMode::Spsc> Inbox; // Messages we read from socket. Runner thread is producer, main thread is consumer.
	TQueue<TArray<uint8>, EQueueMode::Spsc> Outbox; // Messages to send to socket. Main thread is producer, runner thread is consumer.

public:

	//Constructor / Destructor
	FTcpSocketWorker(FString inIp, const int32 inPort, ATcpSocketConnection* InOwner, int32 inId, int32 inRecvBufferSize, int32 inSendBufferSize, float inTimeBetweenTicks);
	virtual ~FTcpSocketWorker();

	/*  Starts processing of the connection. Needs to be called immediately after construction	 */
	void Start();

	/* Adds a message to the outgoing message queue */
	void AddToOutbox(TArray<uint8> Message);

	/* Reads a message from the inbox queue */
	TArray<uint8> ReadFromInbox();

	// Begin FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	// End FRunnable interface	

	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	void SocketShutdown();

	/* Getter for bConnected */
	bool isConnected();

private:
	/* Blocking send */
	bool BlockingSend(const uint8* Data, int32 BytesToSend);

	/** thread should continue running */
	bool bRun = false;

	/** Critical section preventing multiple threads from sending simultaneously */
	FCriticalSection SendCriticalSection;
};