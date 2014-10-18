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
