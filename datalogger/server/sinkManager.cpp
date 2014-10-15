#include <stdlib.h>
#include <iostream>
#include "sinkManager.hpp"
#include "sink.hpp"


SinkManager *SinkManager::inst_ = NULL;


SinkManager::SinkManager()
{
    atexit(&burySingleton);
}

SinkManager::~SinkManager()
{
    for (SinkMap_t::iterator it = sinks_.begin(); it != sinks_.end(); ++it) {
        delete it->second;
    }
}


// Singleton<SinkManager> SinglSinkManager(new SinkManager);
// Singleton<SinkManager> SinglSinkManager;

SinkManager *
SinkManager::instance(void)
{
    if (!inst_) {
        inst_ = new SinkManager;
        if (!inst_) {
            abort();
        }
    }

    return inst_;
}


void
SinkManager::burySingleton(void)
{
    if (inst_) {
        delete inst_;
        inst_ = NULL;
    }
}


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


Sink *
SinkManager::sinkGet(std::string const &sinkName)
{
    SinkMap_t::const_iterator const it = sinks_.find(sinkName);
    if (sinks_.end() == it) {
        return NULL;
    }

    return it->second;
}
