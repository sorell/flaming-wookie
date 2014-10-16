#ifndef HOMEWORK_SERVER_TCP_SOURCE_HPP
#define HOMEWORK_SERVER_TCP_SOURCE_HPP

#include <map>
#include "sink.hpp"

class Observer;


/*---- Class ----------------------------------------------------------------
  Does:
    Manage TCP connection with the outside world. Accept new connections and
    pass Record requests to Sink that is bound by bindSink().
----------------------------------------------------------------------------*/
class TcpSource
{
public:
    TcpSource(Observer *obs = NULL);
    ~TcpSource();

    bool open(int port);
    void blockingListen(void);
    void stop(void) { stop_ = true; }

    //
    // This is for optimization purpose and niftyness. We could also save the sink pointer and refer to
    // pSink->impl()->write().
    //
    void bindSink(Sink *const sink) {
        processRecord_ = sink->processRecFunc();
    }

    typedef std::function<void(Record const &)> RecordSend_f;

private:

    // Data start delimeter for TCP stream
    #define DATA_START_WORD  ((unsigned short) 0x5A5A)
    #define RX_BUFFER_SIZE   1500

    /*---- Struct ---------------------------------------------------------------
      Does:
        Contains peer data (receive buffer) of a client connectee.
    ----------------------------------------------------------------------------*/
    struct ClientConnection {
        ClientConnection() : rxBuffer(new char[RX_BUFFER_SIZE]), rxPos(0), observerConnected(false) {}
        
        // Prevent to use copy constructor by coder's mistake
        ClientConnection(ClientConnection const &) { abort(); }

        // Move constructor is the preferred method: Steal the buffer from the copy source.
        ClientConnection(ClientConnection &&rhs) : rxBuffer(rhs.rxBuffer), rxPos(0) {
            rhs.rxBuffer = NULL;
        }

        ~ClientConnection() { if (rxBuffer) delete [] rxBuffer; }

        
        char *rxBuffer;
        int rxPos;

        bool observerConnected;
    };

    typedef std::map<int, ClientConnection> ClientMap_t;
    ClientMap_t clients_;

    void onClientConnect(int peer);
    void onClientDisconnect(ClientMap_t::iterator connIt);

    int scanForStart(char const *buffer, int dataSize) const;
    int recvfromClient(int socket, ClientConnection &conn);
    int sendToClient(Record const &, uint64_t const priv) const;


    int socket_;
    int port_;
    bool stop_;

    fd_set readFds_;
    int fdmax_;

    Sink::ProcessRecord_f processRecord_;

    Observer *const observer_;
};


#endif  // HOMEWORK_SERVER_TCP_SOURCE_HPP
