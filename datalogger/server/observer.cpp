#include <iostream>
#include "observer.hpp"


bool 
Observer::attachLurker(Record const &rec, uint64_t const id)
{
    bool const ret = lurkers_.insert(Lurkers_t::value_type(id, rec)).second;

    if (!ret) {
        std::cerr << "BUG! when attaching lurker " << id << ": not found" << std::endl;
        abort();
    }

    std::cout << "Added observer (" << rec.serial << ", " << rec.devType << ") for handle " << id << std::endl;
    return true;
}


bool
Observer::detachLurker(uint64_t const id)
{
    Lurkers_t::iterator const it = lurkers_.find(id);

    if (lurkers_.end() == it) {
        std::cerr << "BUG! when detaching lurker " << id << ": not found" << std::endl;
        abort();
    }

    lurkers_.erase(it);
    std::cout << "Removed observer for handle " << id << std::endl;
    return true;
}


int
Observer::relayRec(Record const &rec, Sink::SendRecord_f const &send) const
{
    int count = 0;

    for (Lurkers_t::const_iterator it = lurkers_.begin(); it != lurkers_.end(); ++it) {
fprintf(stderr, "%s, %d: trymatch for %lu\n", __FILE__, __LINE__, it->first);
        if (rec.match(it->second)  &&    send(rec, it->first)) {
            ++count;
        }
    }

    return lurkers_.size();
}
