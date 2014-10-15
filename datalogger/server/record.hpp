#ifndef HOMEWORK_SERVER_RECORD_HPP
#define HOMEWORK_SERVER_RECORD_HPP

#include <sys/time.h>
#include <string>


#define REC_ACT_REPLY             0x0000
#define REC_ACT_STORE             0x0001
#define REC_ACT_GET_AFTER         0x0002
#define REC_ACT_UNDEFINED         0xFFFF


struct Record
{
    Record() : timestamp({0, 0}), action(REC_ACT_UNDEFINED), priv(0) {}
    
    bool validate(void) const {
        switch (action) {
        case REC_ACT_REPLY:
            return true;
            
        case REC_ACT_STORE:
            return devType.length() > 0  &&  serial.length() > 0  &&  data.length() > 0;

        case REC_ACT_GET_AFTER:
            return timestamp.tv_sec != 0  &&  devType.length() > 0  &&  serial.length() > 0;

        default:
            break;
        }
        return false;
    }

    struct timeval timestamp;
    
    // unsigned short action;
    uint16_t action;

    std::string devType;
    std::string serial;
    std::string data;

    uint64_t priv;
};


#endif  // HOMEWORK_SERVER_RECORD_HPP
