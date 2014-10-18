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

#include <iostream>
#include "observer.hpp"


/*---- Function -------------------------------------------------------------
  Does:
    Add a new Lurker to watch added Records.
  
  Wants:
    Reference Record to match with new stored Records.
    Private data 'id' that is used to identify the Lurker at the Source's 
    end.
    
  Gives: 
    True on success.
----------------------------------------------------------------------------*/
bool 
Observer::attachLurker(Record const &rec, uint64_t const id)
{
    std::pair<Lurkers_t::iterator, bool> const ret(lurkers_.insert(Lurkers_t::value_type(id, rec)));

    if (!ret.second) {
        ret.first->second = rec;
        std::cout << "Updated observer (" << rec.serial << ", " << rec.devType << ") for handle " << id << std::endl;
        return true;
    }

    std::cout << "Added observer (" << rec.serial << ", " << rec.devType << ") for handle " << id << std::endl;
    return true;
}


/*---- Function -------------------------------------------------------------
  Does:
    Remove Lurker.
  
  Wants:
    Private data 'id' that identifies previously attached Lurker.
    
  Gives: 
    True on success.
    Will abort() on bug.
----------------------------------------------------------------------------*/
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


/*---- Function -------------------------------------------------------------
  Does:
    Compare stored Record with every Lurker's reference. Send the stored
    record to those clients whose reference Record matches.
  
  Wants:
    Reference Record to match with new stored Records.
    Send function to use to send the Record.
    
  Gives: 
    Number of Records sent.
----------------------------------------------------------------------------*/
int
Observer::relayRec(Record const &rec, Sink::SendRecord_f const &send) const
{
    int count = 0;

    for (Lurkers_t::const_iterator it = lurkers_.begin(); it != lurkers_.end(); ++it) {
        if (rec.match(it->second)  &&    send(rec, it->first)) {
            ++count;
        }
    }

    return lurkers_.size();
}
