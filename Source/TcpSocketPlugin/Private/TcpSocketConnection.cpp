// SpartanTools 2019

#include "TcpSocketConnection.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "IPAddress.h"
#include "Sockets.h"
#include "HAL/RunnableThread.h"
#include "Async/Async.h"
#include <string>
#include "Logging/MessageLog.h"

// Sets default values
ATcpSocketConnection::ATcpSocketConnection()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATcpSocketConnection::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATcpSocketConnection::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TArray<int32> keys;
	TcpWorkers.GetKeys(keys);

	for (auto &key : keys)
	{
		Disconnect(key);
	}
}

// Called every frame
void ATcpSocketConnection::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATcpSocketConnection::Connect(const FString& ipAddress, int32 port, const FTcpSocketDisconnectDelegate& OnDisconnected, const FTcpSocketConnectDelegate& OnConnected,
	const FTcpSocketReceivedMessageDelegate& OnMessageReceived, int32& ConnectionId)
{
	DisconnectedDelegate = OnDisconnected;
	ConnectedDelegate = OnConnected;
	MessageReceivedDelegate = OnMessageReceived;

	ConnectionId = TcpWorkers.Num();

	TSharedRef<FTcpSocketWorker> worker(new FTcpSocketWorker(ipAddress, port, this, ConnectionId, ReceiveBufferSize, SendBufferSize, TimeBetweenTicks));
	
	TcpWorkers.Add(ConnectionId, worker);
	worker->Start();
}

void ATcpSocketConnection::Disconnect(int32 ConnectionId)
{	
	auto worker = TcpWorkers.Find(ConnectionId);
	if (worker)
	{
		UE_LOG(LogTemp, Log, TEXT("Tcp Socket: Disconnected from server."));
		worker->Get().SocketShutdown();
		TcpWorkers.Remove(ConnectionId);
	}
}

bool ATcpSocketConnection::SendData(int32 ConnectionId /*= 0*/, TArray<uint8> DataToSend)
{
	if (TcpWorkers.Contains(ConnectionId))
	{
		if (TcpWorkers[ConnectionId]->isConnected())
		{
			TcpWorkers[ConnectionId]->AddToOutbox(DataToSend);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Log: Socket %d isn't connected"), ConnectionId);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Log: SocketId %d doesn't exist"), ConnectionId);
	}
	return false;
}

void ATcpSocketConnection::ExecuteOnMessageReceived(int32 ConnectionId)
{
	TArray<uint8> msg = TcpWorkers[ConnectionId]->ReadFromInbox();
	MessageReceivedDelegate.ExecuteIfBound(ConnectionId, msg);
}

TArray<uint8> ATcpSocketConnection::Concat_BytesBytes(TArray<uint8> A, TArray<uint8> B)
{
	TArray<uint8> ArrayResult;

	for (int i = 0; i < A.Num(); i++)
	{
		ArrayResult.Add(A[i]);
	}

	for (int i = 0; i < B.Num(); i++)
	{
		ArrayResult.Add(B[i]);
	}

	return ArrayResult;
}

TArray<uint8> ATcpSocketConnection::Conv_IntToBytes(int32 InInt)
{
	TArray<uint8> result;
	for (int i = 0; i < 4; i++)
	{
		result.Add(InInt >> i * 8);
	}
	return result;
}

TArray<uint8> ATcpSocketConnection::Conv_StringToBytes(const FString& InStr)
{
	FString mymessage = InStr;
	TCHAR *messagearray = mymessage.GetCharArray().GetData();
	uint8* message = (uint8*)TCHAR_TO_UTF8(messagearray);

	FTCHARToUTF8 Convert(*mymessage);
	int BytesLength = Convert.Length(); //length of the utf-8 string in bytes (when non-latin letters are used, it's longer than just the number of characters)

	TArray<uint8> result;
	for (int i = 0; i < BytesLength; i++)
	{
		result.Add(message[i]);
	}

	return result;
}

TArray<uint8> ATcpSocketConnection::Conv_FloatToBytes(float InFloat)
{
	TArray<uint8> result;

	unsigned char const * p = reinterpret_cast<unsigned char const *>(&InFloat);
	for (int i = 0; i != sizeof(float); i++)
	{
		result.Add((uint8)p[i]);
	}
	return result;		
}

TArray<uint8> ATcpSocketConnection::Conv_ByteToBytes(uint8 InByte)
{
	TArray<uint8> result{ InByte };
	return result;
}

int32 ATcpSocketConnection::Message_ReadInt(TArray<uint8>& Message)
{
	if (Message.Num() < 4)
	{
		PrintToConsole("Error in the ReadInt node. Not enough bytes in the Message.", true);
		return -1;
	}

	int result;
	unsigned char byteArray[4];

	for (int i = 0; i < 4; i++)
	{
		byteArray[i] = Message[0];		
		Message.RemoveAt(0);
	}

	FMemory::Memcpy(&result, byteArray, 4);
	
	return result;
}

uint8 ATcpSocketConnection::Message_ReadByte(TArray<uint8>& Message)
{
	if (Message.Num() < 1)
	{
		PrintToConsole("Error in the ReadByte node. Not enough bytes in the Message.", true);
		return 255;
	}

	uint8 result = Message[0];
	Message.RemoveAt(0);
	return result;
}

float ATcpSocketConnection::Message_ReadFloat(TArray<uint8>& Message)
{
	if (Message.Num() < 4)
	{
		PrintToConsole("Error in the ReadFloat node. Not enough bytes in the Message.", true);
		return -1.f;
	}

	float result;
	unsigned char byteArray[4];

	for (int i = 0; i < 4; i++)
	{
		byteArray[i] = Message[0];
		Message.RemoveAt(0);
	}

	FMemory::Memcpy(&result, byteArray, 4);

	return result;
}

FString ATcpSocketConnection::Message_ReadString(TArray<uint8>& Message, int32 BytesLength)
{
	if (BytesLength <= 0)
	{
		PrintToConsole("Error in the ReadString node. BytesLength isn't a positive number.", true);
		return FString("");
	}
	if (Message.Num() < BytesLength)
	{
		PrintToConsole("Error in the ReadString node. Message isn't as long as BytesLength.", true);
		return FString("");
	}

	TArray<uint8> StringAsArray;
	StringAsArray.Reserve(BytesLength);

	for (int i = 0; i < BytesLength; i++)
	{
		StringAsArray.Add(Message[0]);
		Message.RemoveAt(0);
	}

	std::string cstr(reinterpret_cast<const char*>(StringAsArray.GetData()), StringAsArray.Num());	
	return FString(UTF8_TO_TCHAR(cstr.c_str()));
}

void ATcpSocketConnection::PrintToConsole(FString Str, bool Error)
{
	if (Error)
	{
		auto messageLog = FMessageLog("Tcp Socket Plugin");
		messageLog.Open(EMessageSeverity::Error, true);
		messageLog.Message(EMessageSeverity::Error, FText::AsCultureInvariant(Str));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Log: %s"), *Str);
	}
}

void ATcpSocketConnection::ExecuteOnConnected(int32 WorkerId)
{
	ConnectedDelegate.ExecuteIfBound(WorkerId);
}

void ATcpSocketConnection::ExecuteOnDisconnected(int32 WorkerId)
{
	DisconnectedDelegate.ExecuteIfBound(WorkerId);
}

bool FTcpSocketWorker::isConnected()
{
	FScopeLock ScopeLock(&SendCriticalSection);
	return bConnected;
}

FTcpSocketWorker::FTcpSocketWorker(FString inIp, const int32 inPort, ATcpSocketConnection* InOwner, int32 inId, int32 inRecvBufferSize, int32 inSendBufferSize, float inTimeBetweenTicks)
	: ipAddress(inIp)
	, port(inPort)
	, ThePC(InOwner)
	, id(inId)
	, RecvBufferSize(inRecvBufferSize)
	, SendBufferSize(inSendBufferSize)
	, TimeBetweenTicks(inTimeBetweenTicks)
{
	
}

FTcpSocketWorker::~FTcpSocketWorker()
{
	AsyncTask(ENamedThreads::GameThread, []() {	ATcpSocketConnection::PrintToConsole("Tcp socket thread was destroyed.", false); });
}

void FTcpSocketWorker::Start()
{
	check(!Thread && "Thread wasn't null at the start!");
	check(FPlatformProcess::SupportsMultithreading() && "This platform doesn't support multithreading!");	
	if (Thread)
	{
		UE_LOG(LogTemp, Log, TEXT("Log: Thread isn't null. It's: %s"), *Thread->GetThreadName());
	}
	Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("FTcpSocketWorker %s:%d"), *ipAddress, port), 128 * 1024, TPri_Normal);
	UE_LOG(LogTemp, Log, TEXT("Log: Created thread"));
}

void FTcpSocketWorker::AddToOutbox(TArray<uint8> Message)
{
	Outbox.Enqueue(Message);
}

TArray<uint8> FTcpSocketWorker::ReadFromInbox()
{
	TArray<uint8> msg;
	Inbox.Dequeue(msg);
	return msg;
}

bool FTcpSocketWorker::Init()
{
	bRun = true;
	bConnected = false;
	return true;
}

uint32 FTcpSocketWorker::Run()
{
	AsyncTask(ENamedThreads::GameThread, []() {	ATcpSocketConnection::PrintToConsole("Starting Tcp socket thread.", false); });

	while (bRun)
	{
		// Connect
		if (!bConnected)
		{
			Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
			if (!Socket)
			{
				return 0;
			}

			Socket->SetReceiveBufferSize(RecvBufferSize, ActualRecvBufferSize);
			Socket->SetSendBufferSize(SendBufferSize, ActualSendBufferSize);

			FIPv4Address ip;
			FIPv4Address::Parse(ipAddress, ip);

			TSharedRef<FInternetAddr> internetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			internetAddr->SetIp(ip.Value);
			internetAddr->SetPort(port);

			bConnected = Socket->Connect(*internetAddr);
			if (bConnected) 
			{
				AsyncTask(ENamedThreads::GameThread, [this]() {
					ThePC->ExecuteOnConnected(id);
				});
			}
			else 
			{
				AsyncTask(ENamedThreads::GameThread, []() { ATcpSocketConnection::PrintToConsole(FString::Printf(TEXT("Couldn't connect to server. TcpSocketConnection.cpp: line %d"), __LINE__), true); });
				bRun = false;				
			}
			continue;
		}

		if (!Socket)
		{
			AsyncTask(ENamedThreads::GameThread, []() { ATcpSocketConnection::PrintToConsole(FString::Printf(TEXT("Socket is null. TcpSocketConnection.cpp: line %d"), __LINE__), true); });
			bRun = false;
			continue;
		}


		// check if we weren't disconnected from the socket
		Socket->SetNonBlocking(true);		
		int32 t_BytesRead;
		uint8 t_Dummy;
		if (!Socket->Recv(&t_Dummy, 1, t_BytesRead, ESocketReceiveFlags::Peek))
		{
			bRun = false;
			continue;
		}
		Socket->SetNonBlocking(false);	

		// if Outbox has something to send, send it
		if (!Outbox.IsEmpty())
		{
			TArray<uint8> toSend; 
			Outbox.Dequeue(toSend);

			if (!BlockingSend(toSend.GetData(), toSend.Num()))
			{
				bRun = false;
				continue;
			}
		}

		// if we can read something
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		uint32 PendingDataSize = 0;
		TArray<uint8> receivedData;

		int32 BytesReadTotal = 0;
		// keep going until we have no data.
		for (;;)
		{
			if (!Socket->HasPendingData(PendingDataSize))
			{
				// no messages
				break;
			}

			ATcpSocketConnection::PrintToConsole(FString::Printf(TEXT("Pending data %d"), (int32)PendingDataSize), false);

			receivedData.SetNumUninitialized(BytesReadTotal + PendingDataSize);

			int32 BytesRead = 0;
			if (!Socket->Recv(receivedData.GetData() + BytesReadTotal, ActualRecvBufferSize, BytesRead))
			{
				// error code: (int32)SocketSubsystem->GetLastErrorCode()
				AsyncTask(ENamedThreads::GameThread, []() {
					ATcpSocketConnection::PrintToConsole(FString::Printf(TEXT("In progress read failed. TcpSocketConnection.cpp: line %d"), __LINE__), true);
				});
				break;
			}
			BytesReadTotal += BytesRead;

			/* TODO: if we have more PendingData than we could read, continue the while loop so that we can send messages if we have any, and then keep recving*/
		}

		// if we received data, inform the main thread about it, so it can read TQueue
		if (receivedData.Num() != 0)
		{
			Inbox.Enqueue(receivedData);
			AsyncTask(ENamedThreads::GameThread, [this]() {
				ThePC->ExecuteOnMessageReceived(id);
			});			
		}

		FPlatformProcess::Sleep(TimeBetweenTicks);
	}

	bConnected = false;

	AsyncTask(ENamedThreads::GameThread, [this]() {
		ThePC->ExecuteOnDisconnected(id);
	});
	
	return 0;
}

void FTcpSocketWorker::Stop()
{
	bRun = false;
}

void FTcpSocketWorker::Exit() 
{
	
}

bool FTcpSocketWorker::BlockingSend(const uint8* Data, int32 BytesToSend)
{
	if (BytesToSend > 0)
	{
		int32 BytesSent = 0;
		if (!Socket->Send(Data, BytesToSend, BytesSent))
		{
			return false;
		}
	}
	return true;
}

void FTcpSocketWorker::SocketShutdown()
{
	if (bRun)
	{
		bRun = false;
	}

	// let the thread shutdown on its own
	if (Thread != nullptr)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}

	// if there is still a socket, close it so our peer will get a quick disconnect notification
	if (Socket)
	{
		Socket->Close();
	}
}