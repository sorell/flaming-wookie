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

    virtual bool open(std::string const &opts) = 0;

    virtual ProcessRecord_f processRecFunc(void) const = 0;

private:
    // No copying the singleton
    Sink(Sink &);
    Sink(Sink &&);
    Sink &operator = (Sink const &);
};

#endif  // HOMEWORK_SERVER_SINK_HPP
