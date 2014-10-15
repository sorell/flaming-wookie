
#ifndef HOMEWORK_SERVER_PROTOCOL_HPP
#define HOMEWORK_SERVER_PROTOCOL_HPP

#include <string.h>
#include <arpa/inet.h>
#include <string>
#include "record.hpp"


#define PRM_ACTION      0x0001
#define PRM_SERNUM      0x0002
#define PRM_DEVTYPE     0x0003
#define PRM_DATA        0x0004
#define PRM_TIME        0x0005



struct RecordData {
    unsigned short type;
    unsigned short len;
    
    union {
        uint16_t u16;
        char d[1];
        struct {
            uint32_t sec;
            uint32_t usec;
        } time;
    } value;

} __attribute__((__packed__));

#define REC_MINSIZE 4  // type + len bytes




struct ActionValidator { bool operator () (RecordData const &data) const { return ntohs(data.len) == 2; } };
struct SernumValidator { bool operator () (RecordData const &data) const { return ntohs(data.len) <= 10; } };
struct DevtypeValidator { bool operator () (RecordData const &data) const { return ntohs(data.len) <= 6; } };
struct DataFieldValidator { bool operator () (RecordData const &data) const { return ntohs(data.len) <= 80; } };  // By the lack of better specs, defined by Stetson-Harrison
struct TimeValidator { bool operator () (RecordData const &data) const { return ntohs(data.len) == 8; } };

struct Copy2Bytes { int operator () (unsigned short &dst, RecordData const &data) const { dst = ntohs(data.value.u16); return 2; } };
struct CopyStr { int operator () (std::string &dst, RecordData const &data) const { dst.assign(data.value.d, (std::string::size_type) ntohs(data.len)); return ntohs(data.len); } };
struct CopyTime { int operator () (struct timeval &dst, RecordData const &data) const { dst.tv_sec = ntohl(data.value.time.sec); dst.tv_usec = ntohl(data.value.time.usec); return sizeof(data.value.time); } };


template <class VALIDATOR, class COPIER>
class ParamParser
{
public:
    ParamParser(VALIDATOR const &v = VALIDATOR(), COPIER const &c = COPIER()) : validator_(v), copier_(c) {}

    template <typename T>
    int extract(T &dst, RecordData const &data, int const dataLeft) const
    {
        if (dataLeft < REC_MINSIZE + ntohs(data.len)) {
            return 0;
        }
        if (!validator_(data)) {
            return -1;
        }
        return REC_MINSIZE + copier_(dst, data);
    }

private:
    VALIDATOR const validator_;
    COPIER const copier_;
};


class Protocol
{
public:
    int deserialize(Record &rec, char const *const buffer, int const dataSize) const
    {
        int processed = 0;
        int ret;
        RecordData *data;


        while (processed + REC_MINSIZE < dataSize) {
            data = (RecordData *) (buffer + processed);
            unsigned short const type = ntohs(data->type);

            switch (type) {
            case PRM_ACTION:
                if (rec.action != REC_ACT_UNDEFINED) {
                    std::cerr << "Invalid data: Record action defined twice" << std::endl;
                    return -1;
                }
                ret = actionParser_.extract(rec.action, *data, dataSize - processed);
                break;

            case PRM_SERNUM:
                if (rec.serial.length() > 0) {
                    std::cerr << "Invalid data: Record serial defined twice" << std::endl;
                    return -1;
                }
                ret = sernumParser_.extract(rec.serial, *data, dataSize - processed);
                break;

            case PRM_DEVTYPE:
                if (rec.devType.length() > 0) {
                    std::cerr << "Invalid data: Record dev type defined twice" << std::endl;
                    return -1;
                }
                ret = devtypeParser_.extract(rec.devType, *data, dataSize - processed);
                break;

            case PRM_DATA:
                if (rec.data.length() > 0) {
                    std::cerr << "Invalid data: Record data defined twice" << std::endl;
                    return -1;
                }
                ret = dataFieldParser_.extract(rec.data, *data, dataSize - processed);
                break;

            case PRM_TIME:
                if (rec.timestamp.tv_sec != 0) {
                    std::cerr << "Invalid data: Record time defined twice" << std::endl;
                    return -1;
                }
                ret = timeParser_.extract(rec.timestamp, *data, dataSize - processed);
                break;

            default:
                // Packet ended?
                // std::cerr << "Invalid protocol param type " << type << std::endl;
                return processed;
            }

            if (ret < 0) {
                std::cerr << "Invalid data extraction of type " << type << std::endl;
                return -1;
            }

            // There was not enough data to decode the TVL?
            if (0 == ret) {
                return 0;
            }

            processed += ret;
        }

        return processed;
    }


    int serialize(char *const buffer, int const bufSize, Record const &rec) const
    {
        RecordData *data = (RecordData *) buffer;
        char *ptr = buffer;


        data->type = PRM_ACTION;
        data->len = 2;
        data->value.u16 = REC_ACT_REPLY;

        ptr += REC_MINSIZE + data->len;
        data = (RecordData *) ptr;

        data->type = PRM_SERNUM;
        data->len = rec.serial.length();
        rec.serial.copy(data->value.d, rec.serial.length());

        ptr += REC_MINSIZE + data->len;
        data = (RecordData *) ptr;

        data->type = PRM_DEVTYPE;
        data->len = rec.devType.length();
        rec.devType.copy(data->value.d, rec.devType.length());

        ptr += REC_MINSIZE + data->len;
        data = (RecordData *) ptr;

        data->type = PRM_DATA;
        data->len = rec.data.length();
        rec.data.copy(data->value.d, rec.data.length());

        ptr += REC_MINSIZE + data->len;
        return ptr - buffer;
    }


private:
    ParamParser<ActionValidator, Copy2Bytes> const actionParser_;
    ParamParser<SernumValidator, CopyStr> const sernumParser_;
    ParamParser<DevtypeValidator, CopyStr> const devtypeParser_;
    ParamParser<DataFieldValidator, CopyStr> const dataFieldParser_;
    ParamParser<TimeValidator, CopyTime> const timeParser_;
};


#endif  // HOMEWORK_SERVER_PROTOCOL_HPP
