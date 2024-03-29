// ECUSerial.cpp : define o ponto de entrada para o aplicativo do console.
// FIAT TIPO 1.6ie BOSCH MONOMOTRONIC MA1.7

#include "ECUMonomotronic.h"
#include "SerialPort.h"
#include "stdafx.h"
#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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
                    for (int i = 0; i < initPackets.size(); i++) {
                        if (initPackets[i].frametypeid == 0xF6) {
                            std::string str(initPackets[i].data.begin(),
                                            initPackets[i].data.end());

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
                    std::optional<std::deque<ECUmmpacket>> errorsList =
                        ECUMgr.ECUReadErrors();
                    if (errorsList) {
                        std::cout << "Read errors sent" << std::endl;

                        for (ECUmmpacket &e : errorsList.value()) {
                            std::cout << "ECU frame nº " << std::dec
                                      << (int)e.counter << std::endl;
                            std::cout << "ECU frame type " << std::hex
                                      << (int)e.frametypeid << std::endl;
                            std::cout << "ECU frame data ";

                            for (auto &b : e.data) {
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

                if (std::optional<std::deque<ECUmmpacket>> sensorData =
                        ECUMgr.ECUReadSensor(0x63)) {
                    for (ECUmmpacket &e : sensorData.value()) {
                        if (e.frametypeid != 9) {
                            std::cout << "ECU frame nº " << std::dec
                                      << (int)e.counter << std::endl;
                            std::cout << "ECU frame type " << std::hex
                                      << (int)e.frametypeid << std::endl;
                            std::cout << "ECU frame data ";

                            for (auto &b : e.data) {
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

                while (true) {
                    if (std::optional<std::deque<ECUmmpacket>> sensorData =
                            ECUMgr.ECUReadSensor(0x62)) {
                        for (ECUmmpacket &e : sensorData.value()) {
                            if (e.frametypeid != 9) {
                                std::cout << "ECU frame nº " << std::dec
                                          << (int)e.counter << std::endl;
                                std::cout
                                    << "ECU frame type " << std::hex
                                    << (int)e.frametypeid << " "
                                    << ECUMgr.getFrameTypeNameStr(e.frametypeid)
                                    << std::endl;
                                std::cout << "ECU frame data ";

                                for (auto &b : e.data) {
                                    double result2 = 126.752196049 -
                                                     1.832473888 * b +
                                                     0.010511937 * b * b -
                                                     2.4838E-05 * b * b * b;
                                    std::cout << std::hex << (int)b << " "
                                              << result2 << " ";
                                }

                                std::cout << std::endl;

                                std::cout << std::endl;
                            }
                        }
                    } else {
                        std::cout << "Read sensor data error" << std::endl;
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(100));

                        continueECUActions = false;
                        break;
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

int main() {
    std::cout << "ECUMonomotronic ECUMgr connect to COM1" << std::endl;
    ECUMonomotronic ECUMgr("\\\\.\\COM1");

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
            break;
            ECUMgr.forcestop();
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
