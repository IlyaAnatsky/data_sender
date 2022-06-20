// Test task from NTR
// This is data sender application

#include <boost/filesystem.hpp>
#include <iostream>
#include <thread>
#include "msg_buffer.h"

struct SSenderStatistics 
{
    int sent_number_packages;

    SSenderStatistics() 
    {
        sent_number_packages = 0;
    }
};
static SSenderStatistics recvStat;

//********************************************************************************
//* main() for data sender application                                           *
//********************************************************************************
int main()
{
    std::cout << "\nData sender application is started\n";

    CConfigIni config("config_data_sender.ini");
    config.Init();

    SConfigV configval(config);
    if (configval.incorrect == true)
    {
        return 1;
    }

    std::stringstream confS;
    confS << "local_ip: " << configval.local_ip << std::endl;
    confS << "local_port: " << configval.local_port << std::endl;
    confS << "remote_ip: " << configval.remote_ip << std::endl;
    confS << "remote_port: " << configval.remote_port << std::endl;
    confS << "total_number_packages: " << configval.total_number_packages << std::endl;
    confS << "do_delay_after_aliquot: " << configval.do_delay_after_aliquot << std::endl;
    confS << "delay_after_aliquot_ms: " << configval.delay_after_aliquot_ms << std::endl;
    confS << "sent_delay: " << configval.sent_delay << std::endl;
    confS << "write_file: " << configval.write_file << std::endl;
    confS << "write_hex: " << configval.write_hex << std::endl;
    std::cout << confS.str();

    std::cout << "\nPlease, check the configuration above and press Enter to continue.\n";
    std::cin.ignore();

    using namespace boost::asio;
    io_service service;
    ip::udp::socket sock(service);
    sock.open(ip::udp::v4());

    ip::udp::endpoint local_ep(ip::address::from_string(configval.local_ip), configval.local_port);
    ip::udp::endpoint remote_ep(ip::address::from_string(configval.remote_ip), configval.remote_port);

    sock.bind(local_ep);

    DataSender::CMsgBuffer msgBufferObj(configval);

    for (int indexMsg = 1; indexMsg <= configval.total_number_packages; ++indexMsg)
    {
        void *buffer_p = msgBufferObj.getNewData();
        short bufferLength = msgBufferObj.getSentDataLength();

        sock.send_to(buffer(buffer_p, bufferLength), remote_ep);

        recvStat.sent_number_packages++;

        // Waiting for some time, expected: 10ms, according to task requirements
        std::this_thread::sleep_for(std::chrono::milliseconds(configval.sent_delay));

        // Check for dalay, expected for sent(1000): 10ms, according to task requirements
        if ((indexMsg % configval.do_delay_after_aliquot) == 0)
        {
            std::cout << "Dalay after sent #MSG: " << indexMsg << ", (" << configval.delay_after_aliquot_ms << "ms)"
                      << "\n";

            // Waiting for some time, expected: 10ms, according to task requirements
            std::this_thread::sleep_for(std::chrono::milliseconds(configval.delay_after_aliquot_ms));
        }
    }
    sock.close();
    
    boost::posix_time::ptime now_stat = boost::posix_time::second_clock::local_time();
    std::string time_stat = to_iso_extended_string(now_stat);

    std::stringstream stat;
    stat << "Data sender statistics ("<< time_stat <<"):\n";    
    stat << "Sent total number of packages: " << recvStat.sent_number_packages << "\n";
    stat << "\nConfiguration:\n" << confS.str();
    std::cout << std::endl << stat.str();
    
    std::ofstream fout;
    fout.open("data_sender_stat.log");    
    fout << stat.str();
    fout.close();

    std::cout << "\nData sender application will be stopped. Please, press Enter to continue.\n";              
    std::cin.ignore();
    return 0;
}
