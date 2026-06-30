// ECUSerial.cpp : define o ponto de entrada para o aplicativo do console.
// FIAT TIPO 1.6ie BOSCH MONOMOTRONIC MA1.7

#include "ECUMonomotronic.h"
#include "ECUSerialCLI.h"
#include "SensorDecoders.h"
#include "SerialPort.h"
#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

struct fundata {
    int id;
    int subcommando;
    uint8_t length;
    bool mustRead;
    // lastTimeRead
    double lastRawDataRead;
    double lastDataRead;
    std::function<double(int)> decoder_fun;

    fundata(int i, int sub, std::function<double(int)> fun, uint8_t len = 1)
        : id(i), subcommando(sub), length(len), decoder_fun(fun) {
        mustRead = false;
    }

    fundata() {
        id = 0;
        subcommando = 0;
        length = 1;
        lastRawDataRead = 0;
        lastDataRead = 0.0;
        mustRead = false;
    }
};

std::unordered_map<std::string, fundata> makeECUFunctions() {
    std::unordered_map<std::string, fundata> map;
    auto decoders = GetSensorDecoders();
    map.reserve(decoders.size());

    for (const auto &entry : decoders) {
        map.emplace(
            std::string(entry.key),
            fundata{entry.id, entry.subcommand, entry.decode, entry.length});
    }

    return map;
}

std::mutex ecuFunctionLock;
std::mutex cgMustRead;
std::unordered_map<std::string, fundata> ECUFunctions = makeECUFunctions();

std::atomic<bool> continueECUActions = true;

void commandThread(ECUMonomotronic &ECUMgr) {
    bool initPrinted = false;
    bool readedErrors = false;

    while (continueECUActions) {
        if (ECUMgr.canAcceptCommands()) {
            while (ECUMgr.isThreadRunning() && continueECUActions) {
                if (!initPrinted) {
                    std::cout << std::endl;

                    std::cout << "ECU Init identify packets:" << std::endl;
                    auto initPackets = ECUMgr.getinitPackets();
                    // CppCoreGuidelines: Use size_t for span indexing
                    for (size_t i = 0; i < initPackets.size(); i++) {
                        if (initPackets[i].frametypeid == 0xF6) {
                            // NASA P10: Use span to access valid data only
                            auto data_span = initPackets[i].get_data();
                            std::string str(data_span.begin(), data_span.end());

                            std::cout << str << std::endl;
                        }
                    }

                    std::cout << std::endl;

                    initPrinted = true;
                }

                /*

Wed Dec 31 22:34:44 1969 971932 WRITE: 6
Wed Dec 31 22:34:44 1969 981288 READ: f9
Wed Dec 31 22:34:44 1969 983380 WRITE: 10
Wed Dec 31 22:34:44 1969 991474 READ: ef
Wed Dec 31 22:34:44 1969 993609 WRITE: 1
Wed Dec 31 22:34:45 1969 1412 READ: fe
Wed Dec 31 22:34:45 1969 3709 WRITE: 1
Wed Dec 31 22:34:45 1969 12417 READ: fe
Wed Dec 31 22:34:45 1969 14619 WRITE: 0
Wed Dec 31 22:34:45 1969 22467 READ: ff
Wed Dec 31 22:34:45 1969 24671 WRITE: 63   c
Wed Dec 31 22:34:45 1969 33675 READ: 9c
Wed Dec 31 22:34:45 1969 35878 WRITE: 3

                Bug:
Wed Dec 31 22:34:45 1969 84674 READ: 4
Wed Dec 31 22:34:45 1969 87004 WRITE: fb
Wed Dec 31 22:34:45 1969 187695 READ: 4
Wed Dec 31 22:34:45 1969 189896 WRITE: fb
Wed Dec 31 22:34:45 1969 291689 READ: 4
Wed Dec 31 22:34:45 1969 293809 WRITE: fb
Wed Dec 31 22:34:45 1969 394703 READ: 4
Wed Dec 31 22:34:45 1969 397031 WRITE: fb
Wed Dec 31 22:34:45 1969 498709 READ: 4


                */

                if (!readedErrors) {
                    std::optional<ECUResponseCollection> errorsList =
                        ECUMgr.ECUReadErrors();
                    if (errorsList) {
                        std::cout << "Read errors sent" << std::endl;

                        for (const ECUmmpacket &e :
                             errorsList.value().get_packets()) {
                            std::cout << "ECU frame nº " << std::dec
                                      << (int)e.counter << std::endl;
                            std::cout << "ECU frame type " << std::hex
                                      << (int)e.frametypeid << std::endl;
                            std::cout << "ECU frame data size "
                                      << e.get_data().size() << std::endl;
                            std::cout << "ECU frame data ";

                            // NASA P10: Use span to iterate only valid data
                            for (auto &b : e.get_data()) {
                                std::cout << std::hex << (int)b << " ";
                            }

                            std::cout << std::endl;

                            bool present = false;

                            std::cout << "Description: "
                                      << ECUMgr.errorPacketToString(e, present)
                                      << std::endl;
                            std::cout << "Present: " << std::boolalpha
                                      << present << std::endl;
                            std::cout << std::endl;
                        }

                        readedErrors = true;
                    } else {
                        std::cout << "Send unavaible yet\n";
                    }
                }

                /*if (readedErrors) {
                    std::cout << "Read errors already sent" << std::endl;
                    ECUMgr.ECUCleanErrors();
                    ECUMgr.stop();
                    return;
                }*/

                if (std::optional<ECUResponseCollection> sensorData =
                        ECUMgr.readECUMemory(0x00, 0x63, 1)) {
                    for (const ECUmmpacket &e :
                         sensorData.value().get_packets()) {
                        if (e.frametypeid != 9) {
                            std::cout << "ECU frame nº " << std::dec
                                      << (int)e.counter << std::endl;
                            std::cout << "ECU frame type " << std::hex
                                      << (int)e.frametypeid << std::endl;
                            std::cout << "ECU frame data ";

                            // NASA P10: Use span to iterate only valid data
                            for (auto &b : e.get_data()) {
                                double result = 129.949770448 -
                                                1.910061145 * b +
                                                0.011346132 * b * b -
                                                2.6406E-05 * b * b * b;
                                std::cout << std::hex << (int)b << " " << result
                                          << " ";
                            }

                            std::cout << std::endl;

                            std::cout << std::endl;
                        }
                    }
                } else {
                    std::cout << "Read sensor data error" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    continueECUActions = false;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                if (std::optional<ECUResponseCollection> sensorData =
                        ECUMgr.readECUMemory(0, 0xB3, 1)) {
                    for (const ECUmmpacket &e :
                         sensorData.value().get_packets()) {
                        if (e.frametypeid != 9) {
                            std::cout << "ECU frame nº " << std::dec
                                      << (int)e.counter << std::endl;
                            std::cout << "ECU frame type " << std::hex
                                      << (int)e.frametypeid << std::endl;
                            std::cout << "ECU frame data ";

                            for (const auto &byted : e.get_data()) {
                                std::cout << std::hex << (int)byted << " ";
                            }

                            std::cout << std::endl;
                        }
                        std::cout << std::endl;
                        std::cout << std::endl;
                    }
                }

                while (true) {
                    /*for (const auto &[name, func] : ECUFunctions) {
                        if (std::optional<ECUResponseCollection> sensorData =
                                ECUMgr.readECUMemory(
                                    static_cast<uint8_t>(func.subcommando),
                                    static_cast<uint8_t>(func.id),
                                    func.length)) {
                            for (const ECUmmpacket &e :
                                 sensorData.value().get_packets()) {
                                if (e.frametypeid != 9) {
                                    std::cout << "ECU frame nº " << std::dec
                                              << (int)e.counter << std::endl;
                                    std::cout << "ECU frame type " << std::hex
                                              << (int)e.frametypeid << " "
                                              << name << std::endl;
                                    std::cout << "ECU frame data ";

                                    int data = 0;

                                    // NASA P10: Use span for consistent access
                                    auto data_span = e.get_data();
                                    if (data_span.size() >= func.length &&
                                        !data_span.empty()) {
                                        if (func.length == 2) {
                                            data = (data_span[0] << 8) |
                                                   data_span[1];
                                        } else {
                                            data = data_span[0];
                                        }
                                    }
                                    double result2 = func.decoder_fun(data);
                                    std::cout << std::hex << (int)data << " "
                                              << result2 << " ";

                                    std::cout << std::endl;
                                }
                                std::cout << std::endl;
                                std::cout << std::endl;
                            }
                        }
                    }*/

                    if (auto sensorColl = ECUMgr.requestSensorCollection()) {
                        for (const ECUmmpacket &e :
                             sensorColl.value().get_packets()) {
                            if (e.frametypeid != 9) {
                                std::cout << "ECU frame nº " << std::dec
                                          << (int)e.counter << std::endl;
                                std::cout << "ECU frame type " << std::hex
                                          << (int)e.frametypeid << " "
                                          << std::endl;
                                std::cout << "ECU frame data ";

                                for (const auto &byted : e.get_data()) {
                                    std::cout << std::hex << (int)byted << " ";
                                }

                                std::cout << std::endl;
                            }
                            std::cout << std::endl;
                            std::cout << std::endl;
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                int waitcount = 0;

                while (ECUMgr.canAcceptCommands() && continueECUActions) {
                    std::cout << "Send ACK...\n";
                    ECUMgr.sendECURequest(ECUMgr.ECU_ACK_CODE);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));

                    while (true) {
                        std::optional<ECUmmpacket> opt =
                            ECUMgr.getECUResponse();

                        if (opt) {
                            std::cout << "Received "
                                      << (int)opt.value().frametypeid << " "
                                      << (int)opt.value().counter << "\n";
                            std::this_thread::sleep_for(
                                std::chrono::milliseconds(50));
                            break;
                        } else if (waitcount > 30) {
                            std::cout << "Fail\n";
                            continueECUActions = false;
                            break;
                        }
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(50));
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }
}

int ctrlc_handler(int sig) {
    std::cout << "Caught signal " << sig << ", stopping ECU actions...\n";
    continueECUActions = false;

    return 0;
}

int main(int argc, char **argv) {
    const std::vector<std::string_view> args = MakeCLIArgViews(argc, argv);
    const ECUSerialCLIParseResult parse_result = ParseECUSerialCLIArgs(args);

    if (parse_result.status == ECUSerialCLIParseStatus::HelpRequested) {
        std::cout << parse_result.message;
        return 0;
    }

    if (parse_result.status == ECUSerialCLIParseStatus::Error) {
        std::cerr << parse_result.message << std::endl;
        return 1;
    }

    const ECULinkConfig &config = parse_result.config;

    std::cout << "Using port: " << config.port << std::endl;
    std::cout << "Using baud: " << ToString(config.session_baud) << std::endl;
    if (config.profile.has_value()) {
        std::cout << "Using profile: " << ToString(config.profile.value())
                  << std::endl;
    }

    std::cout << "ECUMonomotronic ECUMgr connect to " << config.port
              << std::endl;
    ECUMonomotronic ECUMgr(config);

    /*while (true)
    {
            std::string command;
            std::cin >> command;

            if (command == "start")
            {
                    break;
            }
    }*/

    if (ECUMgr.portIsOpen()) {
        std::cout << "ECUMgr.portIsOpen() TRUE" << std::endl;
        ECUMgr.init();
    } else {
        std::cout << "ECUMgr.portIsOpen() FALSE" << std::endl;
    }

    // std::cout << "SetThreadPriority " <<
    // SetThreadPriority(thr.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) <<
    // std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    std::thread th(commandThread, std::ref(ECUMgr));

    ECUMgr.shouldTryAutoConnect(true);

    while (ECUMgr.isThreadRunning()) {
        std::string command;

        std::cout << "Trying to send new command\n";

        std::cin >> command;

        if (command == "stop") {
            continueECUActions = false;
            break;
            ECUMgr.stop();
        } else if (command == "forcestop") {
            continueECUActions = false;
            ECUMgr.forcestop();
            break;
        }

        // printlogging();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        {
            // printlogging();
        }
        // printlogging();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ECUMgr.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    continueECUActions = false;

    if (th.joinable())
        th.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ECUMgr.debugTofile();

    system("pause");

    return 0;
}
