// MonomotronicMA17.cpp : Este arquivo contém a função 'main'. A execução do
// programa começa e termina ali.
//

#include "ECUMonomotronic.h"
#include "SensorDecoders.h"
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

std::unique_ptr<ECUMonomotronic> ECUManager;

std::thread internalCommThread;

std::atomic<bool> continueECUActions = true;
std::atomic<bool> ECUMgrPrinted = false;

std::mutex guardResponse;

std::atomic<int> request = 2;

std::string ECUResponsestr = "";
std::string ECUWelcome;

struct ECUConstantReadBools {
    std::mutex lck;
    bool temperatura_agua;
    bool temperatura_ar;
    bool lambda;
    bool avanco_ignicao;
    bool bico_injetor;
    bool RPM;
    bool TPS1, TPS2;
    bool atuador_marcha_lenta;

    ECUConstantReadBools() {
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

std::mutex ecufunlock;
std::mutex cgMustRead;
std::map<std::string, fundata> makeECUFunctions() {
    std::map<std::string, fundata> map;
    for (const auto &entry : GetSensorDecoders()) {
        map.emplace(
            std::string(entry.key),
            fundata{entry.id, entry.subcommand, entry.decode, entry.length});
    }
    return map;
}

std::map<std::string, fundata> ECUFunctions = makeECUFunctions();

struct ECUConstantReadData {
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

void commandThread(ECUMonomotronic &ECUMgr) {
    bool initPrinted = false;
    bool readedErrors = false;

    ECUConstantReadData internalData;

    while (continueECUActions) {
        if (ECUMgr.canAcceptCommands()) {
            while (ECUMgr.isThreadRunning() && ECUMgr.isECUConnectedNow() &&
                   continueECUActions) {
                if (!initPrinted) {
                    std::cout << std::endl;

                    std::cout << "ECU Init identify packets:" << std::endl;
                    auto initPackets = ECUMgr.getinitPackets();
                    for (int i = 0; i < initPackets.size(); i++) {
                        if (initPackets[i].frametypeid == 0xF6) {
                            std::string str(initPackets[i].data.begin(),
                                            initPackets[i].data.end());

                            ECUWelcome += str;
                            ECUWelcome += "\n";
                        }
                    }

                    ECUMgrPrinted = true;
                    initPrinted = true;
                }

                int reqval = request;
                if (reqval != -1) {
                    switch (reqval) {
                    case 0: {
                        std::optional<ECUResponseCollection> errorsList =
                            ECUMgr.ECUReadErrors();
                        std::stringstream sstr;
                        if (errorsList) {
                            sstr << "Read errors sent" << std::endl;

                            for (const ECUmmpacket &e :
                                 errorsList->get_packets()) {
                                sstr << "ECU frame nº " << std::dec
                                     << (int)e.counter << std::endl;
                                sstr << "ECU frame type " << std::hex
                                     << (int)e.frametypeid << std::endl;
                                sstr << "ECU frame data ";

                                for (auto &b : e.data) {
                                    sstr << std::hex << (int)b << " ";
                                }

                                sstr << std::endl;

                                bool present = false;

                                sstr << "Description: "
                                     << ECUMgr.errorPacketToString(e, present)
                                     << std::endl;
                                sstr << "Present: " << std::boolalpha << present
                                     << std::endl;
                                sstr << std::endl;
                            }

                            readedErrors = true;
                        } else {
                            sstr << "Send unavaible yet\n";
                        }

                        {
                            std::lock_guard<std::mutex> lck(guardResponse);
                            ECUResponsestr = sstr.str();
                        }

                        request = 2;
                    } break;

                    case 1: {
                        std::optional<ECUmmpacket> clsrep =
                            ECUMgr.ECUCleanErrors();

                        if (clsrep) {
                            std::lock_guard<std::mutex> lck(guardResponse);
                            ECUResponsestr = "Clear errors sent\n";
                        } else {
                            std::lock_guard<std::mutex> lck(guardResponse);
                            ECUResponsestr = "Clear errors failed\n";
                        }

                        request = 2;
                    } break;

                    case 2: {
                        for (auto &dt_pair : ECUFunctions) {
                            auto &dt = dt_pair.second;
                            bool mustRead = false;
                            {
                                std::lock_guard<std::mutex> lck(cgMustRead);
                                mustRead = dt.mustRead;
                            }

                            if (mustRead) {
                                if (std::optional<ECUResponseCollection>
                                        sensorData = ECUMgr.readECUMemory(
                                            static_cast<uint8_t>(
                                                dt.subcommando),
                                            static_cast<uint8_t>(dt.id),
                                            dt.length)) {
                                    for (const ECUmmpacket &e :
                                         sensorData->get_packets()) {
                                        if (e.frametypeid != 9) { // Not an ACK
                                            if (e.data.size() >= dt.length &&
                                                !e.data.empty()) {
                                                int raw = 0;
                                                if (dt.length == 2) {
                                                    raw = (e.data[0] << 8) |
                                                          e.data[1];
                                                } else {
                                                    raw = e.data[0];
                                                }
                                                dt.lastRawDataRead = raw;
                                                dt.lastDataRead =
                                                    dt.decoder_fun(raw);
                                            }
                                        }
                                    }
                                } else {
                                    MessageBoxA(0, "Read sensor data error",
                                                "ERROR", 0);
                                    std::this_thread::sleep_for(
                                        std::chrono::milliseconds(100));
                                    continueECUActions = false;

                                    break;
                                }
                            }
                        }
                    } break;

                    default:
                        break;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

extern "C" __declspec(dllexport) const char *getECUResponseStr() {
    static std::string tmpstr;

    {
        std::lock_guard<std::mutex> lck(guardResponse);

        if (ECUResponsestr.size() > 0)
            tmpstr = ECUResponsestr;
        else
            tmpstr = "";
    }

    return tmpstr.c_str();
}

extern "C" __declspec(dllexport) void setScanRequest(int req) { request = req; }

extern "C" __declspec(dllexport) const char *getECUWelcome() {
    if (ECUMgrPrinted)
        return ECUWelcome.c_str();

    return "";
}

extern "C" __declspec(dllexport) double setSensorReadState(const char *sensor,
                                                           bool state) {
    std::lock_guard<std::mutex> lck(cgMustRead);

    auto it = ECUFunctions.find(sensor);

    if (it != ECUFunctions.end()) {
        auto &sensorData = (*it).second;
        sensorData.mustRead = state;
        return sensorData.lastDataRead;
    }

    return 0;
}

extern "C" __declspec(dllexport) void endMgrConnection() {
    continueECUActions = false;
    if (ECUManager) {
        ECUManager->stop();
    }
}

extern "C" __declspec(dllexport) bool isPortOpen() {
    if (ECUManager) {
        return ECUManager->portIsOpen();
    }

    return false;
}

extern "C" __declspec(dllexport) bool initECU() {
    ECUMgrPrinted = false;
    continueECUActions = true;

    if (ECUManager) {
        if (internalCommThread.joinable())
            internalCommThread.join();

        ECUManager->init();
        ECUManager->shouldTryAutoConnect(true);

        internalCommThread =
            std::thread(commandThread, std::ref(*ECUManager.get()));
        return true;
    }

    return false;
}

extern "C" __declspec(dllexport) int getThreadError() {
    if (ECUManager) {
        return ECUManager->getECUThreadError();
    }

    return -1;
}

extern "C" __declspec(dllexport) bool isReadyToComunicate() {
    if (ECUManager) {
        return ECUManager->canAcceptCommands() && ECUManager->isThreadRunning();
    }

    return false;
}

extern "C" __declspec(dllexport) bool initECUManager(const char *port) {
    if (!ECUManager) {
        ECUManager = std::make_unique<ECUMonomotronic>(port);

        return ECUManager.get() != nullptr;
    }

    return false;
}

void initDll() {}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason,
                    _In_ LPVOID lpvReserved) {
    switch (fdwReason) {
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

        if (ECUManager) {
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
//   1. Use a janela do Gerenciador de Soluções para adicionar/gerenciar
//   arquivos
//   2. Use a janela do Team Explorer para conectar-se ao controle do
//   código-fonte
//   3. Use a janela de Saída para ver mensagens de saída do build e outras
//   mensagens
//   4. Use a janela Lista de Erros para exibir erros
//   5. Ir Para o Projeto > Adicionar Novo Item para criar novos arquivos de
//   código, ou Projeto > Adicionar Item Existente para adicionar arquivos de
//   código existentes ao projeto
//   6. No futuro, para abrir este projeto novamente, vá para Arquivo > Abrir >
//   Projeto e selecione o arquivo. sln
