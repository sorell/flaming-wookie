#ifndef HOMEWORK_SERVER_BINTXT_SINK_HPP
#define HOMEWORK_SERVER_BINTXT_SINK_HPP

#include "sink.hpp"

struct Record;


class BintxtSinkImpl
{
public:
    BintxtSinkImpl() : file_(NULL) {}
    ~BintxtSinkImpl();

    bool open(char const *filename);
    bool processRec(Record const &rec, Sink::SendRecord_f const &send);

private:
    bool storeRec(Record const &rec) const;
    bool queryRec(Record const &ref, Sink::SendRecord_f const &send) const;
    int readRec(Record &rec, char const *buffer, int dataSize) const;
    bool matchRec(Record const &rec, Record const &ref) const;

    FILE *file_;
};


class BintxtSink : public Sink
{
public:
    BintxtSink() : Sink("bintxt"), pImpl_(NULL) {}
    virtual ~BintxtSink() { if (pImpl_) delete pImpl_; }

    virtual bool open(void);
    
    BintxtSinkImpl const *impl(void) const { return pImpl_; }

    virtual ProcessRecord_f processRecFunc(void) const { return std::bind(&BintxtSinkImpl::processRec, pImpl_, std::placeholders::_1, std::placeholders::_2); }

private:
    BintxtSinkImpl *pImpl_;
};


#endif  // HOMEWORK_SERVER_BINTXT_SINK_HPP
