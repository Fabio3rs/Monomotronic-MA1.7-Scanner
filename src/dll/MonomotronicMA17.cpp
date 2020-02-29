// MonomotronicMA17.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//

#include <iostream>
#include <memory>
#include <Windows.h>
#include "ECUMonomotronic.h"
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <mutex>
#include <functional>
#include <map>

std::unique_ptr<ECUMonomotronic> ECUManager;

std::thread internalCommThread;

std::atomic<bool> continueECUActions = true;
std::atomic<bool> ECUMgrPrinted = false;

std::mutex guardResponse;

std::atomic<int> request = 2;

std::string ECUResponsestr = "";
std::string ECUWelcome;

struct ECUConstantReadBools
{
	std::mutex lck;
	bool temperatura_agua;
	bool temperatura_ar;
	bool lambda;
	bool avanco_ignicao;
	bool bico_injetor;
	bool RPM;
	bool TPS1, TPS2;
	bool atuador_marcha_lenta;

	ECUConstantReadBools()
	{
		temperatura_agua = false;
		temperatura_ar = false;
		lambda = false;
		avanco_ignicao = false;
		bico_injetor = false;
		RPM = false;
		TPS1 = TPS2 = false;
		atuador_marcha_lenta = false;
	}

} constantRBool;

double decodeAirTemperature(int val)
{
	double b = val;
	return 129.949 - 1.91 * b + 0.0113 * pow(b, 2.0) - 2.6406E-05 * pow(b, 3.0);
}

double decodeWaterTemperature(int val)
{
	double b = val;
	return 126.752 - 1.832 * b + 0.0105 * pow(b, 2.0) - 2.4838E-05 * pow(b, 3.0);
}

struct fundata
{
	int id;
	bool mustRead;
	// lastTimeRead
	double lastRawDataRead;
	double lastDataRead;
	std::function<double(int)> decoder_fun;

	fundata(int i, std::function<double(int)> fun) : id(i), decoder_fun(fun)
	{
		mustRead = false;
	}

	fundata()
	{
		id = 0;
		lastRawDataRead = 0;
		lastDataRead = 0.0;
		mustRead = false;
	}
};

std::mutex ecufunlock;
std::mutex cgMustRead;
std::map<std::string, fundata> ECUFunctions = { {"watertemp", fundata{0x62, decodeWaterTemperature} }, {"airtemp", fundata{0x63, decodeAirTemperature} } };

struct ECUConstantReadData
{
	std::mutex lck;
	double temperatura_agua;
	double temperatura_ar;
	double lambda;
	double avanco_ignicao;
	double bico_injetor;
	double RPM;
	double TPS1, TPS2;
	double atuador_marcha_lenta;

} constantRData;

void commandThread(ECUMonomotronic &ECUMgr)
{
	bool initPrinted = false;
	bool readedErrors = false;

	ECUConstantReadData internalData;

	while (continueECUActions)
	{
		if (ECUMgr.canAcceptCommands())
		{
			while (ECUMgr.isThreadRunning() && continueECUActions)
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

							ECUWelcome += str;
							ECUWelcome += "\n";
						}
					}

					ECUMgrPrinted = true;
					initPrinted = true;
				}

				int reqval = request;
				if (reqval != -1)
				{
					switch (reqval)
					{
					case 0:
					{
						std::optional<std::deque<ECUmmpacket>> errorsList = ECUMgr.ECUReadErrors();
						std::stringstream sstr;
						if (errorsList)
						{
							sstr << "Read errors sent" << std::endl;

							for (ECUmmpacket &e : errorsList.value())
							{
								sstr << "ECU frame nº " << std::dec << (int)e.counter << std::endl;
								sstr << "ECU frame type " << std::hex << (int)e.frametypeid << std::endl;
								sstr << "ECU frame data ";

								for (auto &b : e.data)
								{
									sstr << std::hex << (int)b << " ";
								}

								sstr << std::endl;

								bool present = false;

								sstr << "Description: " << ECUMgr.errorPacketToString(e, present) << std::endl;
								sstr << "Present: " << std::boolalpha << present << std::endl;
								sstr << std::endl;
							}

							readedErrors = true;
						}
						else
						{
							sstr << "Send unavaible yet\n";
						}

						{
							std::lock_guard<std::mutex> lck(guardResponse);
							ECUResponsestr = sstr.str();
						}

						request = 2;
					}
						break;

					case 1:
					{
						std::optional<ECUmmpacket> clsrep = ECUMgr.ECUCleanErrors();

						if (clsrep)
						{
							std::lock_guard<std::mutex> lck(guardResponse);
							ECUResponsestr = "Clear errors sent\n";
						}
						else
						{
							std::lock_guard<std::mutex> lck(guardResponse);
							ECUResponsestr = "Clear errors failed\n";
						}

						request = 2;
					}
						break;

					case 2:
					{
						for (auto &dt : ECUFunctions)
						{
							bool mustRead = false;
							{
								std::lock_guard<std::mutex> lck(cgMustRead);
								mustRead = dt.second.mustRead;
							}

							if (mustRead)
							{
								std::lock_guard<std::mutex> lck(ecufunlock);

								if (std::optional<std::deque<ECUmmpacket>> sensorData = ECUMgr.ECUReadSensor(0x63))
								{
									for (ECUmmpacket& e : sensorData.value())
									{
										if (e.frametypeid != 9)
										{
											for (auto& b : e.data)
											{
												dt.second.lastRawDataRead = b;
												dt.second.lastDataRead = dt.second.decoder_fun(b);
											}

										}
									}
								}
								else
								{
									MessageBoxA(0, "Read sensor data error", "ERROR", 0);
									std::this_thread::sleep_for(std::chrono::milliseconds(100));
									continueECUActions = false;

									break;
								}
							}
						}
					}
						break;

					default:
						break;
					}
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}


extern "C" __declspec(dllexport) const char *getECUResponseStr()
{
	static std::string tmpstr;

	std::lock_guard<std::mutex> lck(guardResponse);

	if (ECUResponsestr.size() > 0)
		tmpstr = ECUResponsestr;
	else
		tmpstr = "";

	return tmpstr.c_str();
}

extern "C" __declspec(dllexport) void setScanRequest(int req)
{
	request = req;
}

extern "C" __declspec(dllexport) const char *getECUWelcome()
{
	if (ECUMgrPrinted)
		return ECUWelcome.c_str();

	return "";
}

extern "C" __declspec(dllexport) double setSensorReadState(const char *sensor, bool state)
{
	std::lock_guard<std::mutex> lck(cgMustRead);

	auto it = ECUFunctions.find(sensor);

	if (it != ECUFunctions.end())
	{
		auto& sensorData = (*it).second;
		sensorData.mustRead = state;
		return sensorData.lastDataRead;
	}

	return 0;
}

extern "C" __declspec(dllexport) void endMgrConnection()
{
	continueECUActions = false;
	if (ECUManager)
	{
		ECUManager->stop();
	}
}

extern "C" __declspec(dllexport) bool isPortOpen()
{
	if (ECUManager)
	{
		return ECUManager->portIsOpen();
	}

	return false;
}

extern "C" __declspec(dllexport) bool initECU()
{
	ECUMgrPrinted = false;
	continueECUActions = true;

	if (ECUManager)
	{
		if (internalCommThread.joinable())
			internalCommThread.join();

		ECUManager->init();

		internalCommThread = std::thread(commandThread, std::ref(*ECUManager.get()));
		return true;
	}

	return false;
}

extern "C" __declspec(dllexport) int getThreadError()
{
	if (ECUManager)
	{
		return ECUManager->getECUThreadError();
	}

	return -1;
}

extern "C" __declspec(dllexport) bool isReadyToComunicate()
{
	if (ECUManager)
	{
		return ECUManager->canAcceptCommands() && ECUManager->isThreadRunning();
	}

	return false;
}

extern "C" __declspec(dllexport) bool initECUManager(const char *port)
{
	if (!ECUManager)
	{
		ECUManager = std::make_unique<ECUMonomotronic>(port);

		return ECUManager.get() != nullptr;
	}

	return false;
}

void initDll()
{

}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		initDll();
		break;

	case DLL_PROCESS_DETACH:
		continueECUActions = false;

		if (ECUManager)
			ECUManager->stop();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (internalCommThread.joinable())
			internalCommThread.join();

		if (ECUManager)
		{
			ECUManager.reset();
		}
		break;

	default:
		break;
	}

	return TRUE;
}

// Executar programa: Ctrl + F5 ou Menu Depurar > Iniciar Sem Depuração
// Depurar programa: F5 ou menu Depurar > Iniciar Depuração

// Dicas para Começar: 
//   1. Use a janela do Gerenciador de Soluções para adicionar/gerenciar arquivos
//   2. Use a janela do Team Explorer para conectar-se ao controle do código-fonte
//   3. Use a janela de Saída para ver mensagens de saída do build e outras mensagens
//   4. Use a janela Lista de Erros para exibir erros
//   5. Ir Para o Projeto > Adicionar Novo Item para criar novos arquivos de código, ou Projeto > Adicionar Item Existente para adicionar arquivos de código existentes ao projeto
//   6. No futuro, para abrir este projeto novamente, vá para Arquivo > Abrir > Projeto e selecione o arquivo. sln
