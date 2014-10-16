#ifndef HOMEWORK_SERVER_BINTXT_SINK_HPP
#define HOMEWORK_SERVER_BINTXT_SINK_HPP

#include "sink.hpp"

struct Record;


/*---- Class ----------------------------------------------------------------
  Does:
    Implement Bintxt Sink functionality. Write to and read data from the file
    (database).
----------------------------------------------------------------------------*/
class BintxtSinkImpl
{
public:
    BintxtSinkImpl() : file_(NULL) {}
    ~BintxtSinkImpl();

    bool open(char const *filename);
    int processRec(Record const &rec, Sink::SendRecord_f const &send);

private:
    bool storeRec(Record const &rec) const;
    bool queryRec(Record const &ref, Sink::SendRecord_f const &send) const;
    int readRec(Record &rec, char const *buffer, int dataSize) const;

    FILE *file_;
};


/*---- Class ----------------------------------------------------------------
  Does:
    Intended to be used as singleton.
    Is fired up on application start. Registers its name to SinkManager 
    upon construction.
----------------------------------------------------------------------------*/
class BintxtSink : public Sink
{
public:
    BintxtSink() : Sink("bintxt"), pImpl_(NULL) {}
    virtual ~BintxtSink() { if (pImpl_) delete pImpl_; }

    virtual bool open(void);
    
    BintxtSinkImpl const *impl(void) const { return pImpl_; }


    /*---- Function -------------------------------------------------------------
      Does:
        Creates binding to implementation's functions.
    ----------------------------------------------------------------------------*/
    virtual ProcessRecord_f processRecFunc(void) const { return std::bind(&BintxtSinkImpl::processRec, pImpl_, std::placeholders::_1, std::placeholders::_2); }

private:
    BintxtSinkImpl *pImpl_;
};


#endif  // HOMEWORK_SERVER_BINTXT_SINK_HPP
