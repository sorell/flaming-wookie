#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include "bintxtSink.hpp"
#include "record.hpp"


//
// Singleton bintxtSink. This registers the sink to sinkManager.
//
static BintxtSink const *sinkSingleton = new BintxtSink;


BintxtSinkImpl::~BintxtSinkImpl()
{ 
    if (file_) {
        fclose(file_); 
        file_ = NULL;
        std::cout << "Closed sink file " << std::endl;
    }
}


bool
BintxtSinkImpl::open(char const *const filename)
{
    if (file_) {
        return false;
    }

    file_ = fopen(filename, "a+b");
    return file_ != NULL;
}


bool
BintxtSinkImpl::processRec(Record const &rec, Sink::SendRecord_f const &send)
{
    switch (rec.action) {
    case REC_ACT_STORE:
        storeRec(rec);
        break;

    case REC_ACT_GET_AFTER:
        queryRec(rec, send);
        break;
    }

    return true;
}


bool
BintxtSinkImpl::storeRec(Record const &rec) const
{
    uint32_t tmp;

    tmp = htonl(rec.timestamp.tv_sec);
    fwrite(&tmp, sizeof(tmp), 1, file_);
    tmp = htonl(rec.timestamp.tv_usec);
    fwrite(&tmp, sizeof(tmp), 1, file_);

    tmp = htonl(rec.serial.length());
    fwrite(&tmp, sizeof(tmp), 1, file_);
    fwrite(rec.serial.c_str(), 1, rec.serial.length(), file_);

    tmp = htonl(rec.devType.length());
    fwrite(&tmp, sizeof(tmp), 1, file_);
    fwrite(rec.devType.c_str(), 1, rec.devType.length(), file_);

    tmp = htonl(rec.data.length());
    fwrite(&tmp, sizeof(tmp), 1, file_);
    fwrite(rec.data.c_str(), 1, rec.data.length(), file_);

    return true;
}


bool
BintxtSinkImpl::queryRec(Record const &ref, Sink::SendRecord_f const &send) const
{
    char buffer[1500];
    fseek(file_, 0, SEEK_SET);
    Record rec;
    int bytes = 0;


    while (!feof(file_)) {
        bytes += fread(buffer + bytes, 1, sizeof(buffer) - bytes, file_);
        int bufPos = 0;

        while (1) {
            int const ret = readRec(rec, buffer + bufPos, bytes - bufPos);

            if (0 == ret) {
                bytes -= bufPos;
                memmove(buffer, buffer + bufPos, bytes);
                break;
            }

            if (matchRec(rec, ref)  &&  send(rec, ref.priv) < 0) {
                return false;
            }

            bufPos += ret;

            if (bufPos >= bytes) {
                break;
            }
        }
    }

    return true;
}


bool
BintxtSinkImpl::matchRec(Record const &rec, Record const &ref) const
{
    if (timercmp(&rec.timestamp, &ref.timestamp, > ))
        return false;
    if (ref.devType != "*"  &&  rec.devType != ref.devType)
        return false;
    if (ref.serial != "*"  &&  rec.serial != ref.serial)
        return false;

    return true;
}


int
BintxtSinkImpl::readRec(Record &rec, char const *const buffer, int const dataSize) const
{
    uint32_t tmp;
    char const *ptr = buffer;


    // Theoretical absolute minimum required for a record
    if (dataSize < 5 * (int) sizeof(uint32_t)) {
        return 0;
    }

    // Timestamp: 8 bytes
    rec.timestamp.tv_sec = ntohl(*((uint32_t *) ptr));
    ptr += sizeof(uint32_t);
    rec.timestamp.tv_usec = ntohl(*((uint32_t *) ptr));
    ptr += sizeof(uint32_t);
    
    // Pascal strings, length is 4 bytes
    tmp = ntohl(*((uint32_t *) ptr));
    ptr += sizeof(uint32_t);
    if (dataSize - (ptr - buffer) < tmp)  return 0;
    rec.serial.assign(ptr, tmp);
    ptr += tmp;

    tmp = ntohl(*((uint32_t *) ptr));
    ptr += sizeof(uint32_t);
    if (dataSize - (ptr - buffer) < tmp)  return 0;
    rec.devType.assign(ptr, tmp);
    ptr += tmp;

    tmp = ntohl(*((uint32_t *) ptr));
    ptr += sizeof(uint32_t);
    if (dataSize - (ptr - buffer) < tmp)  return 0;
    rec.data.assign(ptr, tmp);
    ptr += tmp;

    return ptr - buffer;
}


bool
BintxtSink::open(void)
{
    // Could also come in as parameter
    char const filename[] = "filedb.bin";


    if (pImpl_) {
        return false;
    }

    if (NULL == (pImpl_ = new BintxtSinkImpl)) {
        return false;
    }

    if (!pImpl_->open(filename)) {
        std::cerr << "Can't open file " << filename << std::endl;
        abort();
    }

    std::cout << "Opened sink: File " << filename << std::endl;
    return true;
}

