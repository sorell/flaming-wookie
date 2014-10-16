#ifndef HOMEWORK_SERVER_SINK_HPP
#define HOMEWORK_SERVER_SINK_HPP

#include <functional>
#include "sinkManager.hpp"

struct Record;


/*---- Abstract Class -------------------------------------------------------
  Does:
    Base class for all Sinks. Intended to be used as singleton.
    Registers its name to SinkManager at daemon startup, where it can be
    requested from by the user.
----------------------------------------------------------------------------*/
class Sink
{
public:
	typedef std::function<int (Record const &, uint64_t)> SendRecord_f;
	typedef std::function<int (Record const &, SendRecord_f const &)> ProcessRecord_f;

    Sink(char const *const sinkName) { SINKMGR.sinkRegister(sinkName, this); }
    virtual ~Sink() {}

    virtual bool open(void) = 0;

    virtual ProcessRecord_f processRecFunc(void) const = 0;

private:
    // No copying the singleton
    Sink(Sink &);
    Sink(Sink &&);
    Sink &operator = (Sink const &);
};

#endif  // HOMEWORK_SERVER_SINK_HPP
