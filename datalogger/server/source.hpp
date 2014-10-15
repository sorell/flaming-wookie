#ifndef HOMEWORK_SERVER_SINK_HPP
#define HOMEWORK_SERVER_SINK_HPP

#include "sinkManager.hpp"


class Sink
{
public:
    Sink(char const *const sinkName) { SINKMGR->sinkRegister(sinkName, this); }
    virtual ~Sink() {}

    virtual bool open(void) = 0;

private:
};

#endif  // HOMEWORK_SERVER_SINK_HPP
