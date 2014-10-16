#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <string>
#include "sinkManager.hpp"
#include "bintxtSink.hpp"
#include "tcpSource.hpp"
#include "observer.hpp"


// This is for C-style signal handler
static TcpSource *tpcPtr = NULL;

/*---- Signal handler -------------------------------------------------------
  Does:
    Stop the server on interrupting signals
  
  Wants:
    Signal number.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
void 
signalHandler(int const sig)
{
    switch (sig) {
    case SIGHUP:
    case SIGINT:
    case SIGKILL:
        if (tpcPtr) {
            tpcPtr->stop();
        }
        break;

    default:
        break;
    }
}


/*---- Function -------------------------------------------------------------
  Does:
    Print daemon usage to command line stderr.
  
  Wants:
    Nothing.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
static void 
printHelp(void)
{
    std::string allSinks;

    SINKMGR.forEachName( [&allSinks] (std::string const &name) { allSinks += "      "; allSinks += name; allSinks += '\n'; } );

    std::cerr << "Usage: [-o SINK]" << std::endl;
    std::cerr << "  -o SINK      Select sink (database) to use. Built with sinks:" << std::endl;
    std::cerr << allSinks;
}


/*---- Main Function --------------------------------------------------------
  Does:
    Parse command line arguments.
    Open TCP server.
    Select and start the sink (database) to use based on command line input.
  
  Wants:
    Nothing.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
int 
main(int argc, char **argv)
{
    char const opts[] = "i:o:";
    char const defaultSink[] = "bintxt";
    int const defaultTcpPort = 12345;

    char const *sinkName = defaultSink;
    int tcpPort = defaultTcpPort;
    int c;


    while (-1 != (c = getopt(argc, argv, opts))) {
        switch (c) {
        case 'i':
            tcpPort = atoi(optarg);
            break;

        case 'o':
            sinkName = optarg;
            break;

        default:
            printHelp();
            return -1;
        }
    }


    signal(SIGHUP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGKILL, signalHandler);

    // Since TcpSource is local var and Sinks are singleton, it ensures that
    // the port will be closed before the Sinks, preventing calls to a closed
    // Sink.
    Observer observer;
    TcpSource tcp(&observer);
    tpcPtr = &tcp;

    if (!tcp.open(tcpPort)) {
        return -1;
    }


    Sink *const sink = SINKMGR.sinkGet(sinkName);

    if (!sink) {
        std::cerr << "Sink '" << sinkName << "' not recognised" << std::endl;
        printHelp();
        return -1;
    }

    std::cout << "Using sink '" << sinkName << "'" << std::endl;
    
    if (!sink->open()) {
        return -1;
    }

    tcp.bindSink(sink);
    tcp.blockingListen();  // Daemonize here

    tpcPtr = NULL;
    return 0;
}
