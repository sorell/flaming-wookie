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

#ifndef HOMEWORK_SERVER_RECORD_HPP
#define HOMEWORK_SERVER_RECORD_HPP

#include <sys/time.h>
#include <string>


#define REC_ACT_REPLY             0x0000
#define REC_ACT_STORE             0x0001
#define REC_ACT_GET_AFTER         0x0002
#define REC_ACT_OBSERVE           0x0003
#define REC_ACT_UNDEFINED         0xFFFF


/*---- Struct ---------------------------------------------------------------
  Purpose: 
    Representation of Record's application internal data.
  
  Contains:
    The actual data received/stored by daemon (devType, serial, data)
    Timestamp from the moment the Record entered daemon from outside world.
    Action this Record shall perform.
    Private data used by Record's receiver (Source). Used to carry
    information of the Record's sender.
----------------------------------------------------------------------------*/
struct Record
{
    Record() : timestamp({0, 0}), action(REC_ACT_UNDEFINED), priv(0) {}
    
    Record(Record const &rhs) 
    : timestamp(rhs.timestamp), action(rhs.action), devType(rhs.devType), serial(rhs.serial), data(rhs.data), priv(0) {}
    
    Record(Record &&rhs) 
    : timestamp(rhs.timestamp), action(rhs.action), devType(rhs.devType), serial(rhs.serial), data(rhs.data), priv(0) {}
    
    Record &operator = (Record const &rhs) {
        timestamp = rhs.timestamp;
        action = rhs.action;
        devType = rhs.devType;
        serial = rhs.serial;
        data = rhs.data;
        priv = 0;
        return *this;
    }


    /*---- Function -------------------------------------------------------------
      Does:
        Check Record has a valid state.

      Wants:
        Nothing.
        
      Gives: 
        True on success.
    ----------------------------------------------------------------------------*/
    bool validate(void) const 
    {
        switch (action) {
        case REC_ACT_REPLY:
            return true;
            
        case REC_ACT_STORE:
            return devType.length() > 0  &&  serial.length() > 0  &&  data.length() > 0;

        case REC_ACT_GET_AFTER:
        case REC_ACT_OBSERVE:
            return devType.length() > 0  &&  serial.length() > 0;

        default:
            break;
        }
        return false;
    }


    /*---- Function -------------------------------------------------------------
      Does:
        Match record to reference:
        - Record must be newer.
        - Serial and devtype must match, unless wildcard is given.

      Wants:
        Compared record and reference record data.
        
      Gives: 
        True on success.
    ----------------------------------------------------------------------------*/
    bool match(Record const &rhs) const 
    {
        if (timercmp(&timestamp, &rhs.timestamp, < )) {
            fprintf(stderr, "%s, %d: %lu.%lu vs %lu.%lu failed\n", __FILE__, __LINE__, timestamp.tv_sec, timestamp.tv_usec, rhs.timestamp.tv_sec, rhs.timestamp.tv_usec);
            return false;
        }
        if (rhs.devType != "*"  &&  devType != rhs.devType) {
            fprintf(stderr, "%s, %d: failed\n", __FILE__, __LINE__);
            return false;
        }
        if (rhs.serial != "*"  &&  serial != rhs.serial) {
            fprintf(stderr, "%s, %d: failed\n", __FILE__, __LINE__);
            return false;
        }

        return true;
    }

    struct timeval timestamp;
    
    uint16_t action;

    std::string devType;
    std::string serial;
    std::string data;

    uint64_t priv;
};


#endif  // HOMEWORK_SERVER_RECORD_HPP
