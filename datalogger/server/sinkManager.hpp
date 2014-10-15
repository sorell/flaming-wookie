#ifndef HOMEWORK_SERVER_SINK_MANAGER_HPP
#define HOMEWORK_SERVER_SINK_MANAGER_HPP

#include <string>
#include <map>

class Sink;


#define SINKMGR (SinkManager::instance())

class SinkManager
{
public:
    SinkManager();
    ~SinkManager();

    static SinkManager *instance(void);

    void sinkRegister(std::string const &sinkName, Sink *const sink);
    Sink *sinkGet(std::string const &sinkName);

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
