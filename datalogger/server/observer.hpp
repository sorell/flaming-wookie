#ifndef HOMEWORK_SERVER_OBSERVER_HPP
#define HOMEWORK_SERVER_OBSERVER_HPP

#include <map>
#include "sink.hpp"
#include "record.hpp"


/*---- Class ----------------------------------------------------------------
  Does:
----------------------------------------------------------------------------*/
class Observer
{
public:
    Observer() {}
    ~Observer() {}

    bool attachLurker(Record const &rec, uint64_t id);
    bool detachLurker(uint64_t id);

    int relayRec(Record const &rec, Sink::SendRecord_f const &send) const;

private:
    typedef std::map<uint64_t, Record const> Lurkers_t;
    Lurkers_t lurkers_;
};


#endif  // HOMEWORK_SERVER_OBSERVER_HPP
