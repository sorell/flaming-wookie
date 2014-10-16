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
