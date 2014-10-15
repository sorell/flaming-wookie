#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "tcpSource.hpp"
#include "record.hpp"
#include "protocol.hpp"



TcpSource::TcpSource() : socket_(0), port_(0), stop_(false) 
{
    FD_ZERO(&readFds_);
}


TcpSource::~TcpSource()
{ 
    if (socket_) {
        close(socket_); 
        std::cout << "Closed TCP port " << port_ << std::endl;
        socket_ = 0;
        port_ = 0;
    }
}


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


void
TcpSource::blockingListen(void)
{
    struct sockaddr_in peerAddr;
    int fdmax = socket_ + 1;
    int i;
    fd_set fdset;


    while (!stop_) {
        fdset = readFds_;
        int selected = select(fdmax, &fdset, NULL, NULL, NULL);
        int const error = errno;

        if (-1 == selected) {
            if (EINTR == error) {
                std::cout << "Select caught signal" << std::endl;
                return;
            }
            std::cout << "select error: " << strerror(error) << std::endl;
            return;
        }


        for (i=0; i < fdmax && selected > 0; ++i)  if (FD_ISSET(i, &fdset)) {
            --selected;

            if (i == socket_) {
                socklen_t addrSize = sizeof(peerAddr);
                int const peer = accept(socket_, (struct sockaddr *) &peerAddr, &addrSize);
                
                if (peer < 0) {
                    return;
                }

                bool const ret = clients_.insert(ClientMap_t::value_type(peer, ClientConnection())).second;
                if (!ret) {
                    std::cerr << "BUG! Accepted socket that already existed" << std::endl;
                    return;
                }

                FD_SET(peer, &readFds_);
                if (peer >= fdmax) {
                    fdmax = peer + 1;
                }
            }

            else {
                ClientMap_t::iterator const cl = clients_.find(i);
                if (cl == clients_.end()) {
                    std::cerr << "BUG! Client not found for socket " << i << std::endl;
                    abort();
                    continue;
                }

                int const recv = recvfromClient(i, cl->second);

                if (recv < 0) {
                    std::cerr << "Receive error" << strerror(errno) << std::endl;
                    clients_.erase(cl);
                    FD_CLR(i, &readFds_);
                }
                else if (0 == recv) {
                    std::cout << "Connection closed" << std::endl;
                    clients_.erase(cl);
                    FD_CLR(i, &readFds_);
                }
            }
        }

    }
}


void
TcpSource::recordSend(Record const &rec) const
{
    std::cout << "polo\n";
}


int
TcpSource::scanForStart(char const *const buffer, int const dataSize) const
{
    // Would not need htons because DATA_START_WORD is symmetrical, but I like to have it here 
    // for future proofing.
    static unsigned short const startIndicator = htons(DATA_START_WORD);
    char const *const offset = (char const *) memmem(buffer, dataSize, (char const *) &startIndicator, sizeof(startIndicator));
    return !offset ? -1 : offset - buffer;
}


int
TcpSource::recvfromClient(int const socket, ClientConnection &conn)
{
    // Optimize by saving const bind to TcpSource::recordSend
    static Sink::SendRecord_f const sendFunc(std::bind(&TcpSource::sendToClient, this, std::placeholders::_1, std::placeholders::_2));

    int ret;
    int bytes = recv(socket, conn.rxBuffer + conn.rxPos, RX_BUFFER_SIZE - conn.rxPos, 0);


// fprintf(stderr, "recv %d:", bytes);
// for (int i=0; i<bytes; ++i) fprintf(stderr, " %02X", conn.rxBuffer[i]);
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

        conn.rxPos += 2 + ret;  // 2 to pass the DATA_START_WORD

        Record rec;
        int const ret = p.deserialize(rec, conn.rxBuffer + conn.rxPos, bytes - conn.rxPos);

        if (ret < 0) {
            // Skip the data after 'start marker' at rxPos
            conn.rxPos += 2;
            continue;
        }
        
        if (0 == ret) {
            // Data is incomplete. Wait for more.
            break;
        }

        conn.rxPos += ret;

        if (!rec.validate()) {
            std::cerr << "Record didn't pass validation" << std::endl;
            continue;
        }

        rec.priv = socket;
        processRecord_(rec, sendFunc);
    }


    // Move the unhandled data to the start of the buffer
    memmove(conn.rxBuffer, conn.rxBuffer + conn.rxPos, bytes - conn.rxPos);
    conn.rxPos = 0;

    return 1;
}


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
    char buffer[1500];
    
    int const bytes = p.serialize(buffer, sizeof(buffer), rec);
    if (bytes < 0) {
        std::cerr << "Error in serialize" << std::endl;
        return -1;
    }

    int const ret = send(socket, buffer, bytes, 0);
    if (ret < 0) {
        std::cerr << "Error in send: " << strerror(errno) << std::endl;
        return -1;
    }

    return 0;
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


