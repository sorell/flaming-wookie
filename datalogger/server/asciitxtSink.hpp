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

#ifndef HOMEWORK_SERVER_BINTXT_SINK_HPP
#define HOMEWORK_SERVER_BINTXT_SINK_HPP

#include "sink.hpp"

struct Record;


/*---- Class ----------------------------------------------------------------
  Does:
    Servers as an example of another Sink. Currently has no implementation.
----------------------------------------------------------------------------*/
class AsciitxtSinkImpl
{
public:
    AsciitxtSinkImpl() {}
    ~AsciitxtSinkImpl();

    bool open(std::string const &filename);
    int processRec(Record const &rec, Sink::SendRecord_f const &send) { return -1; }

private:
};


/*---- Class ----------------------------------------------------------------
  Does:
    Intended to be used as singleton.
    Is fired up on application start. Registers its name to SinkManager 
    upon construction.
----------------------------------------------------------------------------*/
class AsciitxtSink : public Sink
{
public:
    AsciitxtSink() : Sink("asciitxt"), pImpl_(NULL) {}
    virtual ~AsciitxtSink() { if (pImpl_) delete pImpl_; }

    virtual bool open(std::string const &opts);
    
    AsciitxtSinkImpl const *impl(void) const { return pImpl_; }


    /*---- Function -------------------------------------------------------------
      Does:
        Creates binding to implementation's functions.
    ----------------------------------------------------------------------------*/
    virtual ProcessRecord_f processRecFunc(void) const { return std::bind(&AsciitxtSinkImpl::processRec, pImpl_, std::placeholders::_1, std::placeholders::_2); }

private:
    AsciitxtSinkImpl *pImpl_;
};


#endif  // HOMEWORK_SERVER_BINTXT_SINK_HPP
