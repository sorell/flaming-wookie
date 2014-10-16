#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include "bintxtSink.hpp"
#include "record.hpp"


/*---- Singleton ------------------------------------------------------------
  Does:
    Register the sink to sinkManager on application startup.
----------------------------------------------------------------------------*/
static BintxtSink const *sinkSingleton = new BintxtSink;


/*---- Destructor -----------------------------------------------------------
  Does:
    Called on application termination. Close the database.
----------------------------------------------------------------------------*/
BintxtSinkImpl::~BintxtSinkImpl()
{ 
    if (file_) {
        fclose(file_); 
        file_ = NULL;
        std::cout << "Closed sink file " << std::endl;
    }
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
BintxtSinkImpl::open(char const *const filename)
{
    if (file_) {
        return false;
    }

    file_ = fopen(filename, "a+b");
    return file_ != NULL;
}


/*---- Function -------------------------------------------------------------
  Does:
    Process record received from client based on its 'action'.
  
  Wants:
    Record's data.
    Handler to a function to use to send the reply.
    
  Gives: 
    1 on successful store action, or
    0 on other success, or
    -1 on failure.
----------------------------------------------------------------------------*/
int
BintxtSinkImpl::processRec(Record const &rec, Sink::SendRecord_f const &send)
{
    bool ret = false;


    switch (rec.action) {
    case REC_ACT_STORE:
        if (true == (ret = storeRec(rec))) {
            return 1;
        }
        break;

    case REC_ACT_GET_AFTER:
        ret = queryRec(rec, send);
        break;
    }

    return ret ? 0 : 1;
}


/*---- Function -------------------------------------------------------------
  Does:
    Write one record to database file. Convert from internal structure to the
    database format.
  
  Wants:
    Record's data.
    
  Gives: 
    True on success.
----------------------------------------------------------------------------*/
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


/*---- Function -------------------------------------------------------------
  Does:
    Scan through the database from start and send every record matching the
    given reference. Graciously handle the situations where a record from
    database is cut at the end of the buffer's capacity.
  
  Wants:
    Reference record data.
    Handler to a function to use to send the replies.
    
  Gives: 
    True on success.
----------------------------------------------------------------------------*/
bool
BintxtSinkImpl::queryRec(Record const &reference, Sink::SendRecord_f const &send) const
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

            rec.action = REC_ACT_REPLY;
            if (rec.match(reference)  &&  send(rec, reference.priv) < 0) {
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


/*---- Function -------------------------------------------------------------
  Does:
    Read one record in database format from buffer and store its data into 
    Record structure.
      
  Wants:
    Reference record data.
    Handler to a function to use to send the replies.
    
  Gives: 
    Number of bytes consumed from the buffer, or
    0 in case the buffer's data was incomplete.
----------------------------------------------------------------------------*/
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


/*---- Function -------------------------------------------------------------
  Does:
    Allocate implementation for Bintxt sink. Allow only one instance.
      
  Wants:
    Nothing.

  Gives: 
    True on success.
----------------------------------------------------------------------------*/
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

