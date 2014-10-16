#include <stdlib.h>
#include <iostream>
#include "sinkManager.hpp"
#include "sink.hpp"


SinkManager *SinkManager::inst_ = NULL;


/*---- Constructor ----------------------------------------------------------
  Does:
    Arrange SinkManager termination on program exit. See ~SinkManager().
----------------------------------------------------------------------------*/
SinkManager::SinkManager()
{
    atexit(&burySingleton);
}

/*---- Destructor -----------------------------------------------------------
  Does:
    Force all Sinks to destruct and close their possibly open databases.
----------------------------------------------------------------------------*/
SinkManager::~SinkManager()
{
    for (SinkMap_t::iterator it = sinks_.begin(); it != sinks_.end(); ++it) {
        delete it->second;
    }
}


/*---- Function -------------------------------------------------------------
  Does:
    Return instance to SinkManager singleton. Initialize on call if it
    doesn't exist.
  
  Wants:
    Nothing.
    
  Gives: 
    SinkManager instance.
----------------------------------------------------------------------------*/
SinkManager &
SinkManager::instance(void)
{
    if (!inst_) {
        inst_ = new SinkManager;
        if (!inst_) {
            std::cerr << "Out of memory" << std::endl;
            abort();
        }
    }

    return *inst_;
}


/*---- Function -------------------------------------------------------------
  Does:
    Destroy SinkManager. Called at program termination.
  
  Wants:
    Nothing.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
void
SinkManager::burySingleton(void)
{
    if (inst_) {
        delete inst_;
        inst_ = NULL;
    }
}


/*---- Function -------------------------------------------------------------
  Does:
    Map Sink's object to its name. Allow no duplicates.
  
  Wants:
    Sink's name and pointer to its object.
    
  Gives: 
    Nothing.
----------------------------------------------------------------------------*/
void
SinkManager::sinkRegister(std::string const &sinkName, Sink *const sink)
{
    SinkMap_t::const_iterator const it = sinks_.find(sinkName);
    if (sinks_.end() != it) {
        std::cerr << "Tried to re-register sink '" << sinkName << "'" << std::endl;
        abort();
    }

    sinks_[sinkName] = sink;
}


/*---- Function -------------------------------------------------------------
  Does:
    Map Sink's object to its name. Allow no duplicates.
  
  Wants:
    Sink's name.
    
  Gives: 
    Pointer to Sink object.
----------------------------------------------------------------------------*/
Sink *
SinkManager::sinkGet(std::string const &sinkName)
{
    SinkMap_t::const_iterator const it = sinks_.find(sinkName);
    if (sinks_.end() == it) {
        return NULL;
    }

    return it->second;
}
