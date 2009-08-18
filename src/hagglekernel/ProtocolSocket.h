/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _PROTOCOLSOCKET_H
#define _PROTOCOLSOCKET_H

/*
  Forward declarations of all data types declared in this file. This is to
  avoid circular dependencies. If/when a data type is added to this file,
  remember to add it here.
*/
class ProtocolSocket;


#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>
#include <libcpphaggle/Exception.h>

#include "Protocol.h"
#include "ManagerModule.h"
#include "DataObject.h"
#include "Metadata.h"

#define DEFAULT_SOCKET_BACKLOG 10

using namespace haggle;

/** */
class ProtocolSocket : public Protocol
{
	SOCKET sock;
	bool socketIsRegistered;
        bool nonblock;
	bool registerSocket();
        void setSocketOptions();
    protected:

	/**
           We wrap the common socket functions so that derived classes
           cannot touch the socket and its state directly. This makes it easier 
           to keep a consistent socket state as derived classes can only set the
           state through this well defined interface. Without this interface,
           it is not clear, e.g., which class unregisters the socket with the 
           kernel, and sets other states. In these functions, we can ensure
           correct operation, although derived classes try to do weird things,
           such as, e.g., trying to close a socket twice, or unregister
           an invalid socket.
	*/
        void handleWatchableEvent(const Watchable &wbl);
	bool openSocket(int domain, int type, int protocol, bool registersock = false, bool nonblock = true);
	bool setSocket(SOCKET sock, bool registersock = false);
	void closeSocket();
	bool bindSocket(const struct sockaddr *saddr, socklen_t addrlen);
	virtual bool setListen(int backlog = DEFAULT_SOCKET_BACKLOG);
	SOCKET acceptOnSocket(struct sockaddr *saddr, socklen_t *addrlen);
	bool setNonblock(bool block = false);
        bool isNonblock();
        void closeConnection();
	ProtocolEvent openConnection(const struct sockaddr *saddr, socklen_t addrlen);
	bool setSocketOption(int level, int optname, void *optval, socklen_t optlen);
	ssize_t sendTo(const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
	ssize_t recvFrom(void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);

	bool socketIsOpen() const { return (sock != INVALID_SOCKET); }
        
	/**
           Functions that are overridden from class Protocol.
	*/
	ProtocolEvent waitForEvent(Timeval *timeout = NULL, bool writeevent = false);
	ProtocolEvent waitForEvent(DataObjectRef &dObj, Timeval *timeout = NULL, bool writeevent = false);
	
	ProtocolEvent receiveData(void *buf, size_t len, const int flags, size_t *bytes);
	ProtocolEvent sendData(const void *buf, size_t len, const int flags, size_t *bytes);

    public:
	ProtocolSocket(const ProtType_t _type, const char *_name, InterfaceRef _localIface = NULL,
                       InterfaceRef _peerIface = NULL, const int _flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL, SOCKET _sock = -1);

	virtual ~ProtocolSocket();
	bool hasWatchable(const Watchable &wbl);

	class ConnectException : public ProtocolException
	{
            public:
		ConnectException(const int err = 0, const char* data = "Connect Error") : ProtocolException(err, data) {}
	};
	class ListenException : public ProtocolException
	{
            public:
		ListenException(const int err = 0, const char* data = "Listen Error") : ProtocolException(err, data) {}
	};
	class AcceptException : public ProtocolException
	{
            public:
		AcceptException(const int err = 0, const char* data = "Accept Error") : ProtocolException(err, data) {}
	};
	class SendException : public ProtocolException
	{
            public:
		SendException(const int err = 0, const char* data = "Send Error") : ProtocolException(err, data) {}
	};
	class ReceiveException : public ProtocolException
	{
            public:
		ReceiveException(const int err = 0, const char* data = "Receive Error") : ProtocolException(err, data) {}
	};
	class SocketException : public ProtocolException
	{
            public:
		SocketException(const int err = 0, const char* data = "Socket Error") : ProtocolException(err, data) {}
	};
	class BindException : public ProtocolException
	{
            public:
		BindException(const int err = 0, const char* data = "Bind Error") : ProtocolException(err, data) {}
	};
};

#endif /* _PROTOCOL_H */
