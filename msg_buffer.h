// Test task from NTR
// CMsgBuffer class definition

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <stdint.h>
#include "sender_config_ini.h"

namespace DataSender
{
    constexpr auto size_len = 2;
    constexpr auto number_len = 4;
    constexpr auto time_len = 23;
    constexpr auto md5_len = 16;
    constexpr auto min_data_len = (600 * sizeof(uint16_t));
    constexpr auto max_data_len = (1600 * sizeof(uint16_t));
    constexpr auto header_len = size_len + number_len + time_len + md5_len;
    constexpr auto max_buffer_size = header_len + max_data_len;

    class CMsgBuffer
    {
        uint8_t bufferMsg[max_buffer_size] = { 0 };
        uint16_t dataSize;
        uint32_t numberMsg;

        std::ofstream fout;
        bool write_file = false;
        bool write_hex = false;

    public:
        CMsgBuffer(SConfigV& configval)
        {
            dataSize = min_data_len;
            numberMsg = 0;

            if (configval.write_file == true)
            {
                fout.open("data_sender.log");
                if (fout.is_open() == true)
                {
                    write_file = true;
                    write_hex = configval.write_hex;
                }
            }
        }

        ~CMsgBuffer()
        {
            if (fout.is_open()) fout.close();
        }
        void* getNewData()
        {
            uint16_t* size_p = (uint16_t*)bufferMsg;
            uint32_t* number_p = (uint32_t*)(bufferMsg + size_len);

            if (dataSize < max_data_len)
                dataSize += sizeof(uint16_t);
            else
                dataSize = min_data_len;

            *size_p = htons(dataSize);
            *number_p = htonl(numberMsg++);

            // Set value into payload part of message
            memset(bufferMsg + header_len, (uint8_t)(numberMsg), dataSize);

            // Get timestamp
            using namespace boost::posix_time;
            ptime now = microsec_clock::local_time();
            std::string time_str = to_iso_extended_string(now);
            // Set timestamp into header
            (void)memcpy(bufferMsg + size_len + number_len, time_str.c_str(), time_len);

            // Get MD5
            boost::uuids::detail::md5 boost_md5;
            boost_md5.process_bytes(bufferMsg + header_len, dataSize);
            boost::uuids::detail::md5::digest_type digest;
            boost_md5.get_digest(digest);
            // Set MD5 into header
            uint8_t* md5_p = bufferMsg + size_len + number_len + time_len;
            uint8_t* uint8_digest_p = reinterpret_cast<uint8_t*>(&digest);
            (void)memcpy(md5_p, uint8_digest_p, md5_len);

            // Print to console
            std::stringstream logstr;
            std::string timestemp((char*)(bufferMsg + size_len + number_len), time_len);
            logstr << "Sent: #number message: " << numberMsg
                   << "\tSize: " << dataSize
                   << "\tTime: " << timestemp << std::endl;
            std::cout << logstr.str();

            if (write_file) // Print to log file
            {
                fout << logstr.str();

                if (write_hex)
                {
                    std::string str_msg_hex("");
                    boost::algorithm::hex(bufferMsg, bufferMsg + header_len + dataSize, std::back_inserter(str_msg_hex));
                    fout << "\tdata=" << str_msg_hex;
                }
                fout << std::endl;
                fout.flush();
            }

            return (void*) bufferMsg;
        }

        auto getSentDataLength()
        {
            return dataSize + header_len;
        }
    };
}
