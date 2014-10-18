/*---- Unlicense ------------------------------------------------------------
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include "tcpSource.hpp"
#include "protocol.hpp"
#include "observer.hpp"


/*---- Constructor ----------------------------------------------------------
  Does:
    Just initialize some members.
----------------------------------------------------------------------------*/
TcpSource::TcpSource(Observer *const obs) : socket_(0), port_(0), stop_(false), fdmax_(0), observer_(obs) 
{
    FD_ZERO(&readFds_);
}


/*---- Destructor -----------------------------------------------------------
  Does:
    Close the socket.
----------------------------------------------------------------------------*/
TcpSource::~TcpSource()
{ 
    if (socket_) {
        close(socket_); 
        std::cout << "Closed TCP port " << port_ << std::endl;
        socket_ = 0;
        port_ = 0;
    }
}


/*---- Function -------------------------------------------------------------
  Does:
    Try to open and initialize the TCP socket for listening.
  
  Wants:
    Port number.
    
  Gives: 
    True on success.
----------------------------------------------------------------------------*/
bool
TcpSource::open(int const port)
{
    struct sockaddr_in sockaddr;
    int yes = 1;


    if (socket_ > 0) {
        std::cerr << "Can't open port " << port << ": Port " << port_ << " already open" << std::endl;
        return false;
    }

    socket_ = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == socket_) {
        std::cerr << "Can't open socket" << strerror(errno) << std::endl;
        return false;
    }
    
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        std::cerr << "setsockopt error: " << strerror(errno) << std::endl;
        // Try to continue
    }
    
    if (bind(socket_, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) == -1) {
        std::cerr << "bind error: " << strerror(errno) << std::endl;
        goto error;
    }
    
    if (listen(socket_, 10) == -1) {
        std::cerr << "listen error: " << strerror(errno) << std::endl;
        goto error;
    }
    
    port_ = port;
    fdmax_ = socket_ + 1;
    FD_SET(socket_, &readFds_);

    std::cout << "Opened TCP port " << port_ << " in socket " << socket_ << std::endl;

    return true;

error:
    if (socket_ > 0) {
        close(socket_);
        socket_ = 0;
    }

    return false;
}


/*---- Function -------------------------------------------------------------
  Does:
    Block to listen the socket infinitely. This is the thread's loop.
    Accept incoming connections to the TCP socket.
    Receive data from clients from opened sockets.
    Manage the fd_set on connection open and close.
  
  Wants:
    Nothing.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
void
TcpSource::blockingListen(void)
{
    struct sockaddr_in peerAddr;
    int i;
    fd_set fdset;


    while (!stop_) {
        fdset = readFds_;
        int selected = select(fdmax_, &fdset, NULL, NULL, NULL);
        int const error = errno;

        if (-1 == selected) {
            if (EINTR == error) {
                std::cout << "Select caught signal" << std::endl;
                return;
            }
            std::cout << "select error: " << strerror(error) << std::endl;
            return;
        }


        for (i=0; i < fdmax_ && selected > 0; ++i)  if (FD_ISSET(i, &fdset)) {
            --selected;

            if (i == socket_) {
                socklen_t addrSize = sizeof(peerAddr);
                int const peer = accept(socket_, (struct sockaddr *) &peerAddr, &addrSize);
                
                if (peer < 0) {
                    std::cout << "accept error: " << strerror(error) << std::endl;
                    return;
                }

                onClientConnect(peer);
            }

            else {
                ClientMap_t::iterator const cl = clients_.find(i);
                if (cl == clients_.end()) {
                    std::cerr << "BUG! Client not found for socket " << i << std::endl;
                    abort();
                }

                int const recv = recvFromClient(i, cl->second);

                if (recv < 0) {
                    std::cerr << "Receive error" << strerror(errno) << std::endl;
                    onClientDisconnect(cl);
                }
                else if (0 == recv) {
                    std::cout << "Connection closed" << std::endl;
                    onClientDisconnect(cl);
                }
            }
        }

    }
}


/*---- Function -------------------------------------------------------------
  Does:
    Add client's socket to Connection list. Adjust select()'s fdmax 
    accordingly.
  
  Wants:
    Client socket's number.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
void
TcpSource::onClientConnect(int const peer)
{
    bool const ret = clients_.insert(ClientMap_t::value_type(peer, ClientConnection())).second;
    if (!ret) {
        std::cerr << "BUG! Accepted socket that already existed" << std::endl;
        abort();
    }

    FD_SET(peer, &readFds_);
    if (peer >= fdmax_) {
        fdmax_ = peer + 1;
    }
}


/*---- Function -------------------------------------------------------------
  Does:
    Remove client from all lists and clear its socket from selected set.
    Adjust select()'s fdmax accordingly.
  
  Wants:
    Iterator to client's Connection.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
void
TcpSource::onClientDisconnect(ClientMap_t::iterator const connIt)
{
    int const peer = connIt->first;

    if (observer_  &&  connIt->second.observerConnected) {
        observer_->detachLurker(peer);
    }

    clients_.erase(connIt);
    FD_CLR(peer, &readFds_);

    if (peer == fdmax_ - 1) {
        for (int i = peer - 1; i > 0; --i) {
            if (FD_ISSET(i, &readFds_)) {
                fdmax_ = i + 1;
                break;
            }
        }
    }
}


/*---- Function -------------------------------------------------------------
  Does:
    Scan for Record start marker in the byte stream.
  
  Wants:
    Byte buffer and size of its valid contents.
    
  Gives: 
    Offset in bytes to the next start marker.
----------------------------------------------------------------------------*/
int
TcpSource::scanForStart(char const *const buffer, int const dataSize) const
{
    // Would not need htons because DATA_START_WORD is symmetrical, but I like to have it here 
    // for future proofing.
    static uint16_t const startIndicator = htons(DATA_START_WORD);
    char const *const offset = (char const *) memmem(buffer, dataSize, (char const *) &startIndicator, sizeof(DATA_START_WORD));
    return !offset ? -1 : offset - buffer;
}


/*---- Function -------------------------------------------------------------
  Does:
    Read byte stream from client and deserialize it to Record structure.
    Pass the Record to the Sink for processing.
  
  Wants:
    Socket number.
    Reference to client's Connection structure.
    
  Gives: 
    1 on success, or
    0 on connection close, or
    -1 on read error.
----------------------------------------------------------------------------*/
int
TcpSource::recvFromClient(int const socket, ClientConnection &conn)
{
    // Optimize by saving const bind to TcpSource::sendToClient
    static Sink::SendRecord_f const sendFunc(std::bind(&TcpSource::sendToClient, this, std::placeholders::_1, std::placeholders::_2));

    int ret;
    int bytes = recv(socket, conn.rxBuffer + conn.rxPos, RX_BUFFER_SIZE - conn.rxPos, 0);


// fprintf(stderr, "recv %d:", bytes);
// for (int i=0; i<bytes; ++i) fprintf(stderr, " %02X", (unsigned char) conn.rxBuffer[i]);
// fprintf(stderr, "\n");

    if (bytes < 1) {
        // Return error or 'connection closed'
        return bytes;
    }

    // Add newly read data to the one that was in buffer
    bytes += conn.rxPos;
    // Always start new parsing at the start of the buffer. This is where the 'start' probably is.
    conn.rxPos = 0;

    // Protocol class holds no state, so there is no initialization overhead
    Protocol p;
    while (true) {
        ret = scanForStart(conn.rxBuffer + conn.rxPos, bytes - conn.rxPos);

        if (ret < 0) {
            // There was no 'start marker'
            break;
        }

        conn.rxPos += ret;  // Start is here
        int const tlvStart = conn.rxPos + sizeof(DATA_START_WORD);  // Packet is here

        Record rec;
        int const ret = p.deserialize(rec, conn.rxBuffer + tlvStart, bytes - tlvStart);

        if (ret < 0) {
            // Could not deserialize the packet. Find next start.
            conn.rxPos = tlvStart;
            continue;
        }
        
        if (0 == ret) {
            // Data is incomplete. Wait for more.
            // rxPos must be at 'start marker'.
            break;
        }

        conn.rxPos = tlvStart + ret;

        if (!rec.validate()) {
            continue;
        }

        
        // Store the Record to Observer or to Sink
        if (REC_ACT_OBSERVE == rec.action) {
            if (observer_) {
                observer_->attachLurker(rec, socket);
                conn.observerConnected = true;
            }
        }
        else {
            rec.priv = socket;
            if (processRecord_(rec, sendFunc) == 1  &&  observer_) {
                observer_->relayRec(rec, sendFunc);
            }

            if (REC_ACT_GET_AFTER == rec.action) {
                sendEmptyRecord(socket);
            }
        }
    }


    // Move the unhandled data to the start of the buffer
    int const moveBytes = bytes - conn.rxPos;
    memmove(conn.rxBuffer, conn.rxBuffer + conn.rxPos, moveBytes);
    conn.rxPos = moveBytes;

    return 1;
}


/*---- Function -------------------------------------------------------------
  Does:
    Serialize one Record to buffer and send it to client's socket. 
  
  Wants:
    Record structure.
    Private data containing handle (client socket's number) to the peer's
    data.
    
  Gives: 
    True on success.
----------------------------------------------------------------------------*/
int
TcpSource::sendToClient(Record const &rec, uint64_t const priv) const
{
    int const socket = (int) priv;

    ClientMap_t::const_iterator const cl = clients_.find(socket);
    if (clients_.end() == cl) {
        std::cerr << "Connection to client in socket " << socket << " does not exist" << std::endl;
        return -1;
    }

    Protocol p;
    char buffer[150];

    // This is to suppress warning "dereferencing type-punned pointer will break strict-aliasing rules"
    // when casting char[] to uint16_t*
    struct StartWordInserter { StartWordInserter (void *const ptr) { *((uint16_t *) ptr) = htons(DATA_START_WORD); } };
    StartWordInserter s(buffer);
    
    int const bytes = p.serialize(buffer + sizeof(DATA_START_WORD), sizeof(buffer) - sizeof(DATA_START_WORD), rec);
    if (bytes < 0) {
        std::cerr << "Error in serialize. Buffer overflow?" << std::endl;
        return -1;
    }

// fprintf(stderr, "send %d:", bytes);
// for (int i=0; i<bytes; ++i) fprintf(stderr, " %02X", (unsigned char) buffer[i]);
// fprintf(stderr, "\n");

    int const ret = send(socket, buffer, bytes + sizeof(DATA_START_WORD), 0);
    if (ret < 0) {
        std::cerr << "Error in send: " << strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}


int
TcpSource::sendEmptyRecord(int const socket) const
{
    static Record const emptyRec(REC_ACT_REPLY);

    return sendToClient(emptyRec, socket);
}


#if 0
//
// C -> C++ pthread start wrapper function
//
static void *
startThread(void *const data)
{
    ClientConnection *const conn = (ClientConnection *) data;
    conn->tcp->listenerThread(conn->socket);
    return NULL;
}


void
TcpSource::listenerThread(int const socket)
{
    while (!stop_) {
    }
}


void
TcpSource::wait4Clients(void)
{
    if (!stop_) {
        std::cerr << "BUG! " << __FUNCTION__ << " called before stopping the TCP server" << std::endl;
        return;
    }

    ClientMap_t::iterator it = clients.begin();
    
    while (it != clients.end()) {
        int const socket = it->first;
        ClientConnection &conn = it->second;

        if (this == conn.tcp) {
            close(socket);

            pthread_kill(conn.thread, SIGHUP);
            pthread_join(conn.thread, NULL);

            it = clients.erase(it);
            continue;
        }

        ++it;
    }
}
#endif


#if 0
void
TcpSource::listenToClient(int const peer)
{

    if (clients.find(peer) != clients.end()) {
        std::cerr << "Connection to client in socket " << peer << " already exists" << std::endl;
        return;
    }


    ClientConnection &conn = clients[peer];
    conn.tcp = this;

    if (pthread_create(&conn.thread, NULL, startThread, (void *) &conn) != 0) {
        std::cerr << "Failed to start thread" << std::endl;
        clients.erase(clients.find(peer));
        return;
    }
}
#endif


