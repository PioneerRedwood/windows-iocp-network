#pragma once
#include "ObjectPool.hpp"

enum class IOType : uint32_t
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT,
	IO_DISCONNECT,
	IO_CONNECT,
};

enum class DisconnectReason : uint32_t
{
	DR_NONE,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_COMPLETION_ERROR,
	DR_IO_REQUEST_ERROR,
	DR_SENDFLUSH_ERROR,
};

class Session;

struct OverlappedIOContext
{
	OverlappedIOContext(Session * owner, IOType ioType) : mSessionObject(owner), mIoType(ioType)
	{
		std::memset(&mOverlapped, 0, sizeof(OVERLAPPED));
		std::memset(&mWsaBuf, 0, sizeof(WSABUF));
		mSessionObject->AddRef();
	}

	OVERLAPPED mOverlapped;
	Session* mSessionObject;
	IOType mIoType;
	WSABUF mWsaBuf;
};

struct OverlappedSendContext : public OverlappedIOContext, public ObjectPool<OverlappedSendContext>
{
	OverlappedSendContext(Session* owner) : OverlappedIOContext(owner, IOType::IO_SEND)
	{}
};

struct OverlappedRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedRecvContext>
{
	OverlappedRecvContext(Session* owner) : OverlappedIOContext(owner, IOType::IO_RECV)
	{}
};

struct OverlappedPreRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedPreRecvContext>
{
	OverlappedPreRecvContext(Session* owner) : OverlappedIOContext(owner, IOType::IO_RECV_ZERO)
	{}
};

struct OverlappedDisconnectContext : public OverlappedIOContext, public ObjectPool<OverlappedDisconnectContext>
{
	OverlappedDisconnectContext(Session* owner, DisconnectReason dr)
		: OverlappedIOContext(owner, IOType::IO_DISCONNECT), mDisconnectReason(dr)
	{}

	DisconnectReason mDisconnectReason;
};

struct OverlappedAcceptContext : public OverlappedIOContext, public ObjectPool<OverlappedAcceptContext>
{
	OverlappedAcceptContext(Session* owner) : OverlappedIOContext(owner, IOType::IO_ACCEPT)
	{}
};

struct OverlappedConnectContext : public OverlappedIOContext, public ObjectPool<OverlappedConnectContext>
{
	OverlappedConnectContext(Session* owner)
		: OverlappedIOContext(owner, IOType::IO_CONNECT)
	{}

	DisconnectReason mDisconnectReason;
};

void DeleteIoContext(OverlappedIOContext* context)
{
	if (nullptr == context)
	{
		return;
	}

	context->mSessionObject->ReleaseRef();

	switch (context->mIoType)
	{
	case IOType::IO_SEND:
	{
		delete static_cast<OverlappedSendContext*>(context);
		break;
	}
	case IOType::IO_RECV_ZERO:
	{
		delete static_cast<OverlappedPreRecvContext*>(context);
		break;
	}
	case IOType::IO_RECV:
	{
		delete static_cast<OverlappedRecvContext*>(context);
		break;
	}

	case IOType::IO_DISCONNECT:
	{
		delete static_cast<OverlappedDisconnectContext*>(context);
		break;
	}
	case IOType::IO_ACCEPT:
	{
		delete static_cast<OverlappedAcceptContext*>(context);
		break;
	}
	case IOType::IO_CONNECT:
	{
		delete static_cast<OverlappedConnectContext*>(context);
		break;
	}
	default:
	{
		CRASH_ASSERT(false);
	}
	}
}