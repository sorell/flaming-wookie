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


/*---- Namespace ------------------------------------------------------------
  Contains: 
    Tools for Protocol to serialize and deserialize Record data.
----------------------------------------------------------------------------*/
namespace wdc {  // Stands for Wire Data Converter


    /*---- Struct ---------------------------------------------------------------
      Purpose: 
        Representation of Record's byte level data.
        Record is presented in Type/Len/Value (TLV) format.
    ----------------------------------------------------------------------------*/
    struct BinRec {
        uint16_t type;
        uint16_t len;
        
        union {
            uint16_t u16;
            char d[1];
            struct {
                uint32_t sec;
                uint32_t usec;
            } time;
        } value;

    } __attribute__((__packed__));

    // type + len bytes
    #define REC_MINSIZE 4


    /*---- Data validators ------------------------------------------------------
      Does:
        Ensure that Record TLV's Length and Data are valid for the Type at hand.

      Wants:
        Reference to one TLV.
        
      Gives:
        True on success.
    ----------------------------------------------------------------------------*/
    struct ActionValidator { 
        bool operator () (BinRec const &bin) const { return ntohs(bin.len) == 2; } 
    };

    struct SernumValidator { 
        bool operator () (BinRec const &bin) const { return ntohs(bin.len) <= 10; } 
    };

    struct DevtypeValidator { 
        bool operator () (BinRec const &bin) const { return ntohs(bin.len) <= 6; } 
    };

    struct DataFieldValidator { 
        // Defined by Stetson-Harrison due to lack of better specs
        bool operator () (BinRec const &bin) const { return ntohs(bin.len) <= 80; } 
    };

    struct TimeValidator { 
        bool operator () (BinRec const &bin) const { return ntohs(bin.len) == 8  &&  ntohl(bin.value.time.usec) < 1000000; } 
    };


    /*---- Data copiers ---------------------------------------------------------
      Does:
        Copy and format Record TLV's Value to or from internal Record structure.

      Wants:
        Source and destination data types.
        
      Gives:
        True on success. Some data types might be possible to verify only on 
        decoding stage.
    ----------------------------------------------------------------------------*/
    struct Uint16ToWire { 
        bool operator () (BinRec &bin, uint16_t const &src) const { bin.value.u16 = htons(src); return true; } 
    };

    struct StrToWire { 
        bool operator () (BinRec &bin, std::string const &src) const { src.copy(bin.value.d, src.length()); return true; } 
    };

    struct TimeToWire { 
        bool operator () (BinRec &bin, struct timeval const &src) const { 
            bin.value.time.sec = ntohl(src.tv_sec); bin.value.time.usec = ntohl(src.tv_usec); return true; 
        } 
    };


    struct Uint16FromWire { 
        bool operator () (uint16_t &dst, BinRec const &bin) const { dst = ntohs(bin.value.u16); return true; } 
    };

    struct StrFromWire { 
        bool operator () (std::string &dst, BinRec const &bin) const { 
            dst.assign(bin.value.d, (std::string::size_type) ntohs(bin.len)); return true; 
        } 
    };

    struct TimeFromWire { 
        bool operator () (struct timeval &dst, BinRec const &bin) const { 
            dst.tv_sec = ntohl(bin.value.time.sec); dst.tv_usec = ntohl(bin.value.time.usec); return true; 
        } 
    };


    /*---- Function -------------------------------------------------------------
      Does:
        Determine number of bytes the data type occupies on byte format.

      Wants:
        Reference to the data type.
        
      Gives:
        Number of bytes. Cannot fail.
    ----------------------------------------------------------------------------*/
    template <class T>
    int Wiresize (T const &) { return sizeof(T); }

    template <>
    int Wiresize <std::string> (std::string const &s) { return s.length(); }

    template <>
    int Wiresize <struct timeval> (struct timeval const &s) { return 8; }


    /*---- Class ----------------------------------------------------------------
      Purpose: 
        Assemble TLV (de)serialization functions from Validator, data Serializer
        and Deserializer needed by data Type. 

        See instantations at the end of Protocol.

        This class holds no state. Its purpose is code generation.
    ----------------------------------------------------------------------------*/
    template <class VALIDATOR, class SER, class DESER>
    class TlvParser
    {
    public:
        TlvParser(VALIDATOR const &v = VALIDATOR(), SER const &s = SER(), DESER const &d = DESER()) : validator_(v), serializer_(s), deserializer_(d) {}


        /*---- Function -------------------------------------------------------------
          Does:
            Deserialize one byte level TLV to its designated variable.
          
          Wants:
            Reference to destination data type for the deserialized data.
            Pointer to byte level data and maximum length of its contents.
            
          Gives: 
            Number of bytes consumed from the buffer, or
            0 in case of insufficient data, or
            -1 on error.
        ----------------------------------------------------------------------------*/
        template <typename T>
        int deserialize(T &dst, char const *const buffer, int const dataLeft) const
        {
            BinRec const *const bin = (BinRec const *) buffer;
            int const dataLen = ntohs(bin->len);

            if (dataLeft < REC_MINSIZE + dataLen) {
                return 0;
            }
            if (!validator_(*bin)) {
                return -1;
            }
            if (!deserializer_(dst, *bin)) {
                return -1;
            }
            return REC_MINSIZE + dataLen;
        }


        /*---- Function -------------------------------------------------------------
          Does:
            Deserialize one byte level TLV to its designated variable.
          
          Wants:
            Pointer to buffer for byte level data and its maximum capacity.
            Reference to source data type for the serialized data.
            Data Type code.
            
          Gives: 
            Number of bytes written to the buffer, or
            -1 in case of buffer overrun or unaccepted data.
        ----------------------------------------------------------------------------*/
        template <typename T>
        int serialize(char *const buffer, int const bufLeft, T const &src, uint16_t const dataType) const
        {
            BinRec *const bin = (BinRec *) buffer;
            int const dataLen = Wiresize(src);

            if (bufLeft < REC_MINSIZE + dataLen) {
                return -1;
            }
            
            bin->type = htons(dataType);
            bin->len = htons(dataLen);

            if (!serializer_(*bin, src)) {
                return -1;
            }
            return REC_MINSIZE + dataLen;
        }

    private:
        VALIDATOR const validator_;
        SER const serializer_;
        DESER const deserializer_;
    };

}  // namespace wdc



/*---- Class ----------------------------------------------------------------
  Purpose: 
    Serialize Record data to byte level format or deserialize byte level data
    to a Record structure.

    This class holds no state. Its purpose is code generation.
----------------------------------------------------------------------------*/
class Protocol
{
public:
    /*---- Function -------------------------------------------------------------
      Does:
        Deserialize wrole Record from byte level set of TLV's.
      
      Wants:
        Destination record structure.
        Pointer to byte level data and maximum length of its contents.
        
      Gives: 
        Number of bytes consumed from the buffer, or
        0 in case of insufficient data, or
        -1 on error.
    ----------------------------------------------------------------------------*/
    int deserialize(Record &rec, char const *const buffer, int const dataSize) const
    {
        int processed = 0;
        int ret;


        while (processed + REC_MINSIZE < dataSize) {
            wdc::BinRec *const bin = (wdc::BinRec *) (buffer + processed);
            uint16_t const type = ntohs(bin->type);

            switch (type) {
            case PRM_ACTION:
                if (rec.action != REC_ACT_UNDEFINED) {
                    return processed;  // Sama param again: Must be another record
                }
                
                ret = actionParser_.deserialize(rec.action, buffer + processed, dataSize - processed);

                // Timestamp the Record when deserialized from the byte stream.
                // Do not stamp OBSERVE Records.
                if (REC_ACT_STORE == rec.action) {
                    gettimeofday(&rec.timestamp, NULL);
                }

                break;

            case PRM_SERNUM:
                if (rec.serial.length() > 0) {
                    return processed;  // Sama param again: Must be another record
                }
                ret = sernumParser_.deserialize(rec.serial, buffer + processed, dataSize - processed);
                break;

            case PRM_DEVTYPE:
                if (rec.devType.length() > 0) {
                    return processed;  // Sama param again: Must be another record
                }
                ret = devtypeParser_.deserialize(rec.devType, buffer + processed, dataSize - processed);
                break;

            case PRM_DATA:
                if (rec.data.length() > 0) {
                    return processed;  // Sama param again: Must be another record
                }
                ret = dataFieldParser_.deserialize(rec.data, buffer + processed, dataSize - processed);
                break;

            case PRM_TIME:
                if (rec.timestamp.tv_sec != 0) {
                    return processed;  // Sama param again: Must be another record
                }
                ret = timeParser_.deserialize(rec.timestamp, buffer + processed, dataSize - processed);
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

        return 0;
    }


    /*---- Function -------------------------------------------------------------
      Does:
        Serialize one record to byte level set of TLV's.
      
      Wants:
        Pointer to byte level data and its maximum capacity.
        Source Record structure.
        
      Gives: 
        Number of bytes written to the buffer, or
        -1 in case of buffer overrun or unaccepted data.
    ----------------------------------------------------------------------------*/
    int serialize(char *const buffer, int const bufSize, Record const &rec) const
    {
        int bytes = 0;
        int ret;

        if ((ret = actionParser_.serialize(buffer + bytes, bufSize - bytes, rec.action, PRM_ACTION)) < 0) {
            std::cerr << "Invalid record action" << std::endl;
            return -1;
        }
        bytes += ret;

        if ((ret = sernumParser_.serialize(buffer + bytes, bufSize - bytes, rec.serial, PRM_SERNUM)) < 0) {
            std::cerr << "Invalid record serial" << std::endl;
            return -1;
        }
        bytes += ret;

        if ((ret = devtypeParser_.serialize(buffer + bytes, bufSize - bytes, rec.devType, PRM_DEVTYPE)) < 0) {
            std::cerr << "Invalid record devType" << std::endl;
            return -1;
        }
        bytes += ret;

        if ((ret = dataFieldParser_.serialize(buffer + bytes, bufSize - bytes, rec.data, PRM_DATA)) < 0) {
            std::cerr << "Invalid record data" << std::endl;
            return -1;
        }
        bytes += ret;

        if ((ret = timeParser_.serialize(buffer + bytes, bufSize - bytes, rec.timestamp, PRM_TIME)) < 0) {
            std::cerr << "Invalid record time" << std::endl;
            return -1;
        }
        bytes += ret;

        return bytes;
    }


private:
    wdc::TlvParser <wdc::ActionValidator,    wdc::Uint16ToWire, wdc::Uint16FromWire> const actionParser_;
    wdc::TlvParser <wdc::SernumValidator,    wdc::StrToWire,    wdc::StrFromWire> const    sernumParser_;
    wdc::TlvParser <wdc::DevtypeValidator,   wdc::StrToWire,    wdc::StrFromWire> const    devtypeParser_;
    wdc::TlvParser <wdc::DataFieldValidator, wdc::StrToWire,    wdc::StrFromWire> const    dataFieldParser_;
    wdc::TlvParser <wdc::TimeValidator,      wdc::TimeToWire,   wdc::TimeFromWire> const   timeParser_;
};


#endif  // HOMEWORK_SERVER_PROTOCOL_HPP
