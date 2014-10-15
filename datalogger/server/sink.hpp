#ifndef HOMEWORK_SERVER_SINK_HPP
#define HOMEWORK_SERVER_SINK_HPP

#include <functional>
#include "sinkManager.hpp"

struct Record;


class Sink
{
public:
	typedef std::function<int (Record const &, uint64_t)> SendRecord_f;
	typedef std::function<bool (Record const &, SendRecord_f const &)> ProcessRecord_f;

    Sink(char const *const sinkName) { SINKMGR->sinkRegister(sinkName, this); }
    virtual ~Sink() {}

    virtual bool open(void) = 0;

    virtual ProcessRecord_f processRecFunc(void) const = 0;

private:
};

#endif  // HOMEWORK_SERVER_SINK_HPP
