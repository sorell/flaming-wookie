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
#include "asciitxtSink.hpp"


/*---- Singleton ------------------------------------------------------------
  Does:
    Register the sink to sinkManager on application startup.
----------------------------------------------------------------------------*/
static AsciitxtSink const *sinkSingleton = new AsciitxtSink;


/*---- Destructor -----------------------------------------------------------
  Does:
    Called on application termination. Close the database.
----------------------------------------------------------------------------*/
AsciitxtSinkImpl::~AsciitxtSinkImpl()
{ 
}


/*---- Function -------------------------------------------------------------
  Does:
    Open the datebase file in binary mode for reading and writing.
  
  Wants:
    File name.
    
  Gives: 
    True on success
----------------------------------------------------------------------------*/
bool
AsciitxtSinkImpl::open(std::string const &filename)
{
    std::cerr << "Sorry, no implementation" << std::endl;
    std::cerr << "This is an example of what is required to implement a new Sink" << std::endl;
    abort();
}


/*---- Function -------------------------------------------------------------
  Does:
    Allocate implementation for Bintxt sink. Allow only one instance.
      
  Wants:
    Nothing.

  Gives: 
    True on success.
----------------------------------------------------------------------------*/
bool
AsciitxtSink::open(std::string const &opts)
{
    std::string filename(opts.length() > 0 ? opts : "filedb.txt");


    if (pImpl_) {
        return false;
    }

    if (NULL == (pImpl_ = new AsciitxtSinkImpl)) {
        return false;
    }

    if (!pImpl_->open(filename)) {
        std::cerr << "Can't open file " << filename << std::endl;
        abort();
    }

    std::cout << "Opened sink: File " << filename << std::endl;
    return true;
}

