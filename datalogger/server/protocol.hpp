
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



struct BinData {
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




struct ActionValidator { bool operator () (BinData const &bin) const { return ntohs(bin.len) == 2; } };
struct SernumValidator { bool operator () (BinData const &bin) const { return ntohs(bin.len) <= 10; } };
struct DevtypeValidator { bool operator () (BinData const &bin) const { return ntohs(bin.len) <= 6; } };
struct DataFieldValidator { bool operator () (BinData const &bin) const { return ntohs(bin.len) <= 80; } };  // By the lack of better specs, defined by Stetson-Harrison
struct TimeValidator { bool operator () (BinData const &bin) const { return ntohs(bin.len) == 8; } };

struct Copy2Bytes { int operator () (unsigned short &dst, BinData const &bin) const { dst = ntohs(bin.value.u16); return 2; } };
struct CopyStr { int operator () (std::string &dst, BinData const &bin) const { dst.assign(bin.value.d, (std::string::size_type) ntohs(bin.len)); return ntohs(bin.len); } };
struct CopyTime { int operator () (struct timeval &dst, BinData const &bin) const { dst.tv_sec = ntohl(bin.value.time.sec); dst.tv_usec = ntohl(bin.value.time.usec); return sizeof(bin.value.time); } };


template <class VALIDATOR, class COPIER>
class ParamParser
{
public:
    ParamParser(VALIDATOR const &v = VALIDATOR(), COPIER const &c = COPIER()) : validator_(v), copier_(c) {}

    template <typename T>
    int extract(T &dst, BinData const &bin, int const dataLeft) const
    {
        if (dataLeft < REC_MINSIZE + ntohs(bin.len)) {
            return 0;
        }
        if (!validator_(bin)) {
            return -1;
        }
        return REC_MINSIZE + copier_(dst, bin);
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
        BinData *bin;


        while (processed + REC_MINSIZE < dataSize) {
            bin = (BinData *) (buffer + processed);
            unsigned short const type = ntohs(bin->type);

            switch (type) {
            case PRM_ACTION:
                if (rec.action != REC_ACT_UNDEFINED) {
                    std::cerr << "Invalid data: Record action defined twice" << std::endl;
                    return -1;
                }
                ret = actionParser_.extract(rec.action, *bin, dataSize - processed);
                break;

            case PRM_SERNUM:
                if (rec.serial.length() > 0) {
                    std::cerr << "Invalid data: Record serial defined twice" << std::endl;
                    return -1;
                }
                ret = sernumParser_.extract(rec.serial, *bin, dataSize - processed);
                break;

            case PRM_DEVTYPE:
                if (rec.devType.length() > 0) {
                    std::cerr << "Invalid data: Record dev type defined twice" << std::endl;
                    return -1;
                }
                ret = devtypeParser_.extract(rec.devType, *bin, dataSize - processed);
                break;

            case PRM_DATA:
                if (rec.data.length() > 0) {
                    std::cerr << "Invalid data: Record data defined twice" << std::endl;
                    return -1;
                }
                ret = dataFieldParser_.extract(rec.data, *bin, dataSize - processed);
                break;

            case PRM_TIME:
                if (rec.timestamp.tv_sec != 0) {
                    std::cerr << "Invalid data: Record time defined twice" << std::endl;
                    return -1;
                }
                ret = timeParser_.extract(rec.timestamp, *bin, dataSize - processed);
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
        BinData *bin = (BinData *) buffer;
        char *ptr = buffer;


        bin->type = PRM_ACTION;
        bin->len = 2;
        bin->value.u16 = REC_ACT_REPLY;

        ptr += REC_MINSIZE + bin->len;
        bin = (BinData *) ptr;

        bin->type = PRM_SERNUM;
        bin->len = rec.serial.length();
        rec.serial.copy(bin->value.d, rec.serial.length());

        ptr += REC_MINSIZE + bin->len;
        bin = (BinData *) ptr;

        bin->type = PRM_DEVTYPE;
        bin->len = rec.devType.length();
        rec.devType.copy(bin->value.d, rec.devType.length());

        ptr += REC_MINSIZE + bin->len;
        bin = (BinData *) ptr;

        bin->type = PRM_DATA;
        bin->len = rec.data.length();
        rec.data.copy(bin->value.d, rec.data.length());

        ptr += REC_MINSIZE + bin->len;
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
