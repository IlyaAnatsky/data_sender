// Test task from NTR
// CMsgBuffer class definition

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
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
    constexpr auto min_data_len = 600;
    constexpr auto max_data_len = 1600;
    constexpr auto header_len = size_len + number_len + time_len + md5_len;
    constexpr auto max_buffer_size = header_len + max_data_len;

    class CMsgBuffer
    {
        uint8_t buffer[max_buffer_size] = {0};

        std::ofstream fout;
        bool write_file = false;
        bool write_hex = false;

    public:
        CMsgBuffer(SConfigV &configval)
        {            
            uint16_t *size_p = (uint16_t *) buffer;       
            *size_p = min_data_len;

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
        void *getNewData()
        {
            uint16_t *size_p = (uint16_t *) buffer;
            uint32_t *number_p = (uint32_t *)(buffer + size_len);

            if (*size_p < max_data_len)
                (*size_p)++;
            else
                *size_p = min_data_len;

            (*number_p)++;

            // Set random values into payload part of message
            memset(buffer + header_len, (uint8_t)(*size_p), (*size_p));

            // Get timestamp
            using namespace boost::posix_time;
            ptime now = microsec_clock::local_time();
            std::string time_str = to_iso_extended_string(now);
            // Set timestamp into header
            (void)memcpy(buffer + size_len + number_len, time_str.c_str(), time_len);

            // Get MD5
            boost::uuids::detail::md5 boost_md5;
            boost_md5.process_bytes(buffer + header_len, *size_p);
            boost::uuids::detail::md5::digest_type digest;
            boost_md5.get_digest(digest);            
            // Set MD5 into header
            uint8_t* md5_p = buffer + size_len + number_len + time_len;
            uint8_t* uint8_digest_p = reinterpret_cast<uint8_t*>(&digest);
            (void)memcpy(md5_p, uint8_digest_p, md5_len);

            // Print to console
            std::stringstream logstr;
            std::string timestemp((char *)(buffer + size_len + number_len), time_len);
            logstr << "Sent: #number message: " << (*number_p)
                   << "\tSize: " << (*size_p)
                   << "\tTime: " << timestemp << std::endl;
            std::cout << logstr.str();

            if (write_file) // Print to log file
            {
                fout << logstr.str();
                
                if (write_hex)
                {
                    std::string str_msg_hex("");
                    boost::algorithm::hex(buffer, buffer + (*size_p + header_len), std::back_inserter(str_msg_hex));
                    fout << "\tdata=" << str_msg_hex;
                }
                fout << std::endl;
                fout.flush();
            }            

            return (void *) buffer;
        }

        auto getSentDataLength()
        {
            uint16_t sizeData = *((uint16_t *) buffer);
            return header_len + sizeData;
        }
    };
}
