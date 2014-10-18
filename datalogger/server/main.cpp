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

#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <string>
#include "sinkManager.hpp"
#include "bintxtSink.hpp"
#include "tcpSource.hpp"
#include "observer.hpp"


// Some defaults for cmdline arguments
static char const defaultSink[] = "bintxt";
static int const defaultTcpPort = 12345;

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

    std::cerr << "Usage: [-o SINK[:OPTS]] [-p PORT]" << std::endl;
    std::cerr << "  -o SINK      Select Sink (database) to use. (default " << defaultSink << ")" << std::endl;
    std::cerr << "      Built with sinks:" << std::endl;
    std::cerr << allSinks;
    std::cerr << "  -p PORT      TCP port to listen (default " << defaultTcpPort << ")" << std::endl;
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
    char const opts[] = "hp:o:";

    std::string sinkName(defaultSink);
    std::string sinkOpt;
    int tcpPort = defaultTcpPort;
    int c;


    while (-1 != (c = getopt(argc, argv, opts))) {
        switch (c) {
        case 'p':
            tcpPort = atoi(optarg);
            break;

        case 'o':
            size_t pos;
            sinkName = optarg;

            if ((pos = sinkName.find(':')) != sinkName.npos) {
                sinkOpt = sinkName.substr(pos+1);
                sinkName.erase(pos);
            }
            break;

        case 'h':
            printHelp();
            return 0;

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


    Sink *const sink = SINKMGR.sinkGet(sinkName);

    if (!sink) {
        std::cerr << "Sink '" << sinkName << "' not recognised" << std::endl;
        printHelp();
        return -1;
    }

    tpcPtr = &tcp;
    if (!tcp.open(tcpPort)) {
        return -1;
    }

    std::cout << "Using sink '" << sinkName << "'" << std::endl;
    
    if (!sink->open(sinkOpt)) {
        return -1;
    }

    tcp.bindSink(sink);
    tcp.blockingListen();  // Daemonize here

    tpcPtr = NULL;
    return 0;
}
