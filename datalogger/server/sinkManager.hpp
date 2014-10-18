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

#ifndef HOMEWORK_SERVER_SINK_MANAGER_HPP
#define HOMEWORK_SERVER_SINK_MANAGER_HPP

#include <string>
#include <map>

class Sink;


#define SINKMGR (SinkManager::instance())

/*---- Class ----------------------------------------------------------------
  Does:
    Intended to be used as on-call initializing singleton.
    Simple container to map Sink names to their objects.
----------------------------------------------------------------------------*/
class SinkManager
{
public:
    SinkManager();
    ~SinkManager();

    static SinkManager &instance(void);

    void sinkRegister(std::string const &sinkName, Sink *const sink);
    Sink *sinkGet(std::string const &sinkName);

    /*---- Function -------------------------------------------------------------
      Does:
        Call function f for every Sink name in map.
      
      Wants:
        Fucntion or other callable.
        
      Gives: 
        Nothing.
    ----------------------------------------------------------------------------*/
    template <typename F>
    void forEachName(F const f) const {
        for (SinkMap_t::const_iterator it = sinks_.begin(); it != sinks_.end(); ++it) {
            f(it->first);
        }
    }

private:
    // No copying the singleton
    SinkManager(SinkManager &);
    SinkManager(SinkManager &&);
    SinkManager &operator = (SinkManager const &);

    static void burySingleton(void);
    static SinkManager *inst_;

    typedef std::map<std::string, Sink *> SinkMap_t;
    SinkMap_t sinks_;
};


#endif  // HOMEWORK_SERVER_SINK_MANAGER_HPP
