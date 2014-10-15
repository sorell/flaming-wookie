#ifndef HOMEWORK_SERVER_TCP_SOURCE_HPP
#define HOMEWORK_SERVER_TCP_SOURCE_HPP

#include <map>
#include "sink.hpp"


class TcpSource
{
public:
    TcpSource();
    ~TcpSource();

    bool open(int port);
    void blockingListen(void);
    void stop(void) { stop_ = true; }

    //
    // This is for optimisation purposes. We could also save the sink pointer and refer to
    // pSink->impl()->write().
    //
    void bindSink(Sink *const sink) {
        processRecord_ = sink->processRecFunc();
    }

    typedef std::function<void(Record const &)> RecordSend_f;
    void recordSend(Record const &) const;

    // void listenerThread(int socket);

private:

    // Data start delimeter for TCP stream
    #define DATA_START_WORD  0x5A5A
    #define RX_BUFFER_SIZE   1500

    struct ClientConnection {
        ClientConnection() : rxBuffer(new char[RX_BUFFER_SIZE]), rxPos(0) {}
        
        // Prevent to use copy constructor by coder's mistake
        ClientConnection(ClientConnection const &) { abort(); }

        // Move constructor is the preferred method: Steal the buffer from the copy source.
        ClientConnection(ClientConnection &&rhs) : rxBuffer(rhs.rxBuffer), rxPos(0) {
            rhs.rxBuffer = NULL;
        }

        ~ClientConnection() { if (rxBuffer) delete [] rxBuffer; }

        char *rxBuffer;
        int rxPos;
    };

    int scanForStart(char const *buffer, int dataSize) const;
    int recvfromClient(int socket, ClientConnection &conn);
    int sendToClient(Record const &) const;


    typedef std::map<int, ClientConnection> ClientMap_t;
    ClientMap_t clients_;


    int socket_;
    int port_;
    bool stop_;
    fd_set readFds_;


    Sink::ProcessRecord_f processRecord_;
};


#endif  // HOMEWORK_SERVER_TCP_SOURCE_HPP
