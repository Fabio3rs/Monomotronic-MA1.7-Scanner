// ECUSerial.cpp : define o ponto de entrada para o aplicativo do console.
// FIAT TIPO 1.6ie BOSCH MONOMOTRONIC MA1.7

#include "stdafx.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include "SerialPort.h"
#include <vector>
#include <array>
#include <exception>
#include <fstream>
#include <atomic>
#include <mutex>
#include "ECUMonomotronic.h"

std::atomic<bool> continueECUActions = true;

int main()
{
	std::cout << "ECUMonomotronic ECUMgr connect to COM1" << std::endl;
	ECUMonomotronic ECUMgr("\\\\.\\COM1");

	if (ECUMgr.portIsOpen())
	{
		std::cout << "ECUMgr.portIsOpen() TRUE" << std::endl;
		ECUMgr.init();
	}
	else
	{
		std::cout << "ECUMgr.portIsOpen() FALSE" << std::endl;
	}

	//std::cout << "SetThreadPriority " << SetThreadPriority(thr.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(2500));

	bool initPrinted = false;

	while (continueECUActions)
	{
		std::string command;

		std::cout << "Trying to send new command\n";
		if (ECUMgr.canAcceptCommands())
		{
			if (!initPrinted)
			{
				std::cout << std::endl;

				std::cout << "ECU Init identify packets:" << std::endl;
				auto initPackets = ECUMgr.getinitPackets();
				for (int i = 0; i < initPackets.size(); i++)
				{
					if (initPackets[i].frametypeid == 0xF6)
					{
						std::string str(initPackets[i].data.begin(), initPackets[i].data.end());

						std::cout << str << std::endl;
					}
				}

				std::cout << std::endl;
			}

			if (ECUMgr.sendECURequest(ECUMgr.ECU_READ_ERRORS_CODE))
			{
				std::cout << "Read errors sent" << std::endl;
				std::cout << "Waiting ECU's response" << std::endl;

				std::cout << std::endl;

				std::deque<ECUmmpacket> ecup;
				std::optional<ECUmmpacket> ecuptmp;
				uint8_t frametypeidtmp = 0;

				do
				{
					do
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						ecuptmp = ECUMgr.getECUResponse();
					} while (!ecuptmp.has_value());

					frametypeidtmp = ecuptmp.value().frametypeid;
					std::cout << "ECU's response frame id: " << (int)frametypeidtmp << std::endl;
					if (frametypeidtmp != ECUMgr.ECU_ACK_CODE)
					{
						std::cout << "Sending ACK..." << std::endl;
						bool sendACK = false;
						do
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							sendACK = ECUMgr.sendECURequest(ECUMgr.ECU_ACK_CODE);
						} while (!sendACK);
					}

					ecup.push_back(std::move(ecuptmp.value()));
				} while (frametypeidtmp != ECUMgr.ECU_ACK_CODE);

				std::cout << std::endl;

				for (ECUmmpacket &e : ecup)
				{
					std::cout << "ECU frame nº " << std::dec << (int)e.counter << std::endl;
					std::cout << "ECU frame type " << std::hex << (int)e.frametypeid << std::endl;
					std::cout << "ECU frame data ";

					for (auto &b : e.data)
					{
						std::cout << std::hex << (int)b << " ";
					}

					std::cout << std::endl;

					bool present = false;

					std::cout << "Description: " << ECUMgr.errorPacketToString(e, present) << std::endl;
					std::cout << "Present: " << std::boolalpha << present << std::endl;
					std::cout << std::endl;
				}

				std::cout << "Sending Clear codes..." << std::endl;
				bool sendACK = false;
				do
				{
					sendACK = ECUMgr.sendECURequest(ECUMgr.ECU_CLEAR_ERRORS_CODE);
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				} while (!sendACK);

				std::cout << "Clear codes sent..." << std::endl;

				do
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					ecuptmp = ECUMgr.getECUResponse();
				} while (!ecuptmp.has_value());

				if (ecuptmp.value().frametypeid == ECUMgr.ECU_ACK_CODE)
				{
					std::cout << "ECU accepted clear codes request" << std::endl;
				}
				else
				{
					std::cout << "Error on request" << std::endl;
					std::cout << (int)ecuptmp.value().frametypeid << std::endl;
				}

				break;
			}
			else
			{
				std::cout << "Send unavaible yet\n";
			}
		}

		/*std::cin >> command;

		if (command == "forcestop")
		{
			ECUMgr.forcestop();
		}*/
		//printlogging();

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		{
			//printlogging();
		}
		//printlogging();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ECUMgr.stop();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));


	continueECUActions = false;

	system("pause");

	ECUMgr.debugTofile();

    return 0;
}

