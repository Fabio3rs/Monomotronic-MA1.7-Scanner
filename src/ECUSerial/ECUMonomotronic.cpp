/*
https://github.com/Fabio3rs/Monomotronic-MA1.7-Scanner

MIT License

Copyright (c) 2019 Fabio3rs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "ECUMonomotronic.h"
#include <iostream>
#include <map>

void ECUMonomotronic::updatePacketCounter(ECUMonomotronic &mm, const ECUmmpacket &p)
{
	if (p.counter > mm.ECUPacketCounter)
		mm.ECUPacketCounter = p.counter;
}

void ECUMonomotronic::printlogging()
{
	//std::lock_guard<std::mutex> a(logm);
	for (auto &b : bytesLogging)
	{
		if (b.act == 0)
		{
			std::cout << "READ: " << std::hex << b.byte;

			if (isalnum(b.byte) || isspace(b.byte))
			{
				std::cout << "   " << (char)b.byte;
			}

			std::cout << "\n";
		}
		else
		{
			std::cout << "WRITE: " << std::hex << b.byte;

			if (isalnum(b.byte) || isspace(b.byte))
			{
				std::cout << "   " << (char)b.byte;
			}

			std::cout << "\n";
		}
	}

	bytesLogging.clear();
}

void ECUMonomotronic::fprintlogging(std::fstream &fs)
{
	//std::lock_guard<std::mutex> a(logm);
	for (auto &b : bytesLogging)
	{
		if (b.act == 0)
		{
			fs << "READ: " << std::hex << b.byte;

			if (isalnum(b.byte) || isspace(b.byte))
			{
				fs << "   " << (char)b.byte;
			}

			fs << "\n";
		}
		else
		{
			fs << "WRITE: " << std::hex << b.byte;

			if (isalnum(b.byte) || isspace(b.byte))
			{
				fs << "   " << (char)b.byte;
			}

			fs << "\n";
		}
	}
	bytesLogging.clear();
}

void ECUMonomotronic::debug_regiter_err(const char *file, int line)
{
	debug_file = file;
	debug_line = line;
}

std::optional<ECUmmpacket> ECUMonomotronic::getECUResponse()
{
	if (ECUCommandResultAvailable)
	{
		ECUCommandResultAvailable = false;
		return std::move(ECUResponse);
	}

	return std::nullopt;
}

bool ECUMonomotronic::sendECURequest(uint8_t frameid, const std::vector<uint8_t> &data)
{
	if (ECUThreadCanAcceptCommands)
	{
		std::lock_guard<std::mutex> lck(ECUNewCommandMutex);

		ECUNewCommandTemp.frametypeid = frameid;
		ECUNewCommandTemp.data = data;

		ECUNewCommandAvailable = true;

		return true;
	}
	return false;
}

std::string ECUMonomotronic::errorPacketToString(const ECUmmpacket &p, bool &present)
{
	if (p.frametypeid == ECU_ERROR_DATA_CODE)
	{
		if (p.data.size() >= 3)
		{
			// Source: http://www.fiat-tipo.ru/fpost8823.html
			static const std::map<int, std::string> errList = {
				{ 0x1A01, "Atuador de marcha lenta - Sinal ruim? Motor com do atuador de passo com defeito? Travado?"},
				{ 0x0302, "Sensor de rotação do virabrequim | Sem sinal/motor desligado"},
				{ 0x0402, "Interruptor do atuador de marcha lenta | Curto ao GND ou VCC"},
				{ 0x0602, "Erro TPS"},
				{ 0x0A02, "Sensor de temperatura do liquido de arrefecimento"},
				{ 0x0B02, "Sensor de temperatura do ar circuito aberto ou curto para GND ou VCC" },
				{ 0x0D02, "Sonda lambda | problema sinal"},
				{ 0x3102, "Erro de correção de mistura"},
				{ 0x0303, "Sensor de rotação do virabrequim | Erro de sincronismo do sensor de posição do virabrequim" },
				{ 0x3A46, "Imobilizador | Ativo" },
				{ 0xFFFF, "Erro de ECU | Defeito no computador ou selecionado incorretamente OU problema de conexão" }
			};

			/*
			1E - Error present?
			9E - Error intermitent?
			*/
			int errcode = *(uint16_t*)&p.data[0];
			present = (p.data[2] == 0x1E || p.data[2] == 0x03);

			auto it = errList.find(errcode);

			if (it != errList.end())
			{
				return (*it).second;
			}

			return std::string();
		}
	}
	return std::string();
}

template<int n>
void sendInit(SerialPort &sp, int timeoutms, std::array<bool, n> v)
{
	for (int i = 0; i < v.size(); i++)
	{
		sp.scBreak(v[i]);
		std::this_thread::sleep_for(std::chrono::milliseconds(timeoutms));
	}
}

void ECUMonomotronic::ECUOpenThread()
{
	if (!ECUThreadRunning)
	{
		ECUThreadObj = std::thread(ECUThreadFun, std::ref(*this));
		ECUThreadRunning = true;

		std::cout << "SetThreadPriority " << SetThreadPriority(ECUThreadObj.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) << std::endl;
	}
}

void ECUMonomotronic::sendInitSequence()
{
	sp.scBreak(true);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	sendInit<8>(sp, 200, std::array<bool, 8>{1, 1, 1, 1, 0, 1, 1, 1});
	sp.scBreak(false);
}

void ECUMonomotronic::ECUThreadFun(ECUMonomotronic &mm)
{
	auto lastpackettime = std::chrono::system_clock::now();

	mm.ECUThreadCanAcceptCommands = false;

	while (!mm.ECUThreadExit)
	{
		int state = mm.ECUThreadState;
		bool ECUSendSyncCode = false;

		bool nError = false;

		int key1, key2, key3, key4;

		switch (state)
		{
		case 0: // SEND INIT SEQUENCE / RECEIVE ECU'S WELCOME MESSAGE
		{
			mm.sendInitSequence();

			for (int i = 0; i < 10000 && state != 5; i++)
			{
				char b = 0;
				ECUByte code = mm.ECURead();

				if (code.has_value())
				{
					switch (state)
					{
					case 0:
						if (code == 0x55 && !ECUSendSyncCode)
						{
							ECUSendSyncCode = true;
							++state;
						}
						break;

					case 1:
						key1 = code.value(); // 0x46
						++state;
						break;

					case 2:
						key2 = code.value(); // 0x85
						++state;
						break;

					case 3:
						key3 = code.value(); // 0x89
						++state;
						break;

					case 4:
						key4 = code.value(); // 0xBC
						++state;
						break;

					default:
						break;
					}
				}
			}

			if (ECUSendSyncCode)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(23));
				if (!mm.ECUWrite(~key2))
				{
					mm.debug_regiter_err(__FILE__, __LINE__);
					mm.ECUThreadErr = 2;
					nError = true;
				}
			}
			else
			{
				mm.debug_regiter_err(__FILE__, __LINE__);
				mm.ECUThreadErr = 1;
				nError = true;
			}

			if (!nError)
			{
				int code = 0;
				do
				{
					std::optional<ECUmmpacket> p = mm.ECUReadPacket();
					
					if (p.has_value())
					{
						updatePacketCounter(mm, p.value());
						code = p.value().frametypeid;

						mm.initPackets.push_back(std::move(p.value()));

						if (code == ECU_ACK_CODE)
						{
							break;
						}
						else
						{
							if (!mm.ECUWritePacket(ECU_ACK_CODE))
							{
								mm.debug_regiter_err(__FILE__, __LINE__);
								mm.ECUThreadErr = 4;
								nError = true;
							}
						}
					}
					else
					{
						mm.debug_regiter_err(__FILE__, __LINE__);
						mm.ECUThreadErr = 3;
						nError = true;
						break;
					}
				} while (true);
			}


			lastpackettime = std::chrono::system_clock::now();
			mm.ECUThreadCanAcceptCommands = true;
		}
			mm.ECUThreadState++;
			break;

		case 1: // KEEP THE CONNECTION OPEN AND SEND/RECEIVE CUSTOM REQUESTS
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			if (mm.KeepECUConnectionAlive)
			{
				auto now = std::chrono::system_clock::now();

				std::chrono::duration<double> diff = now - lastpackettime;

				// Custom request handler
				if (mm.ECUNewCommandAvailable)
				{
					std::lock_guard<std::mutex> lck(mm.ECUNewCommandMutex);
					if (!mm.ECUWritePacket(mm.ECUNewCommandTemp.frametypeid, std::move(mm.ECUNewCommandTemp.data)))
					{
						mm.debug_regiter_err(__FILE__, __LINE__);
						mm.ECUThreadErr = 7;
						nError = true;
					}
					else
					{
						std::optional<ECUmmpacket> p = mm.ECUReadPacket();

						if (p.has_value())
						{
							updatePacketCounter(mm, p.value());

							mm.ECUResponse = std::move(p.value());

							lastpackettime = std::chrono::system_clock::now();
							mm.ECUCommandResultAvailable = true;
						}
						else
						{
							mm.debug_regiter_err(__FILE__, __LINE__);
							mm.ECUThreadErr = 9;
							nError = true;
							mm.ECUCommandResultAvailable = false;
						}

						mm.ECUNewCommandAvailable = false;
					}
				}
				else if (diff.count() > 0.5) // Keep the connection alive
				{
					if (!mm.ECUWritePacket(ECU_ACK_CODE))
					{
						mm.debug_regiter_err(__FILE__, __LINE__);
						mm.ECUThreadErr = 5;
						nError = true;
					}
					else
					{
						std::optional<ECUmmpacket> p = mm.ECUReadPacket();

						if (p.has_value())
						{
							mm.ECUPacketCounter = p.value().counter;
							int code = p.value().frametypeid;

							if (code == ECU_ACK_CODE)
							{
								break;
							}
							else
							{
								mm.debug_regiter_err(__FILE__, __LINE__);
								mm.ECUThreadErr = 6;
								nError = true;
							}
						}

						lastpackettime = std::chrono::system_clock::now();
					}
				}
			}

			if (nError)
			{
				mm.ECUThreadState++;
			}
			break;

		default:
			// Debug code
			std::cout << "Default " << mm.ECUThreadErr << std::endl;
			//mm.printlogging();
			if (mm.debug_file != nullptr)
			{
				std::cout << "File error: " << mm.debug_file << std::endl;
				std::cout << "Line error: " << mm.debug_line << std::endl;
			}

			{
				std::fstream fs("ECUSerial.log", std::ios::out | std::ios::trunc);
				mm.fprintlogging(fs);
				fs.flush();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			mm.ECUThreadRunning = false;
			return;
			break;
		}

		std::this_thread::yield();
	}

	mm.ECUThreadRunning = false;
}

bool ECUMonomotronic::ECUWrite(uint8_t b)
{
	b = b & 0xFF;

	if (enableLog)
		addbytelog(blogging(1, static_cast<int>(b) & 0xFF));

	if (!sp.writeByte(b))
	{
		return false;
	}

	std::optional<uint8_t> result;

	do
	{
		result = sp.fastByteReadUSerialPort();
	} while (!result.has_value());

	if (result.has_value())
		return result.value() == b;

	return false;
}

ECUByte ECUMonomotronic::ECURead()
{
	std::optional<uint8_t> result = sp.fastByteReadUSerialPort();

	if (enableLog && result.has_value())
	{
		addbytelog(blogging(0, static_cast<int>(result.value()) & 0xFF));
	}

	return result;
}

ECUByte ECUMonomotronic::ECUReadResponse()
{
	ECUByte result = ECURead();

	if (result.has_value())
	{
		ECUWrite(~result.value());
	}

	return result;
}

bool ECUMonomotronic::ECUWriteResponse(uint8_t b)
{
	if (!ECUWrite(b))
	{
		return false;
	}

	ECUByte result = ECURead();

	if (!result)
	{
		return false;
	}

	return true /*TODO: result.value() == (~b)*/;
}

std::vector<uint8_t> ECUMonomotronic::ECUReadSequential(int size)
{
	std::vector<uint8_t> result;

	result.reserve(size);

	for (int i = 0; i < size; i++)
	{
		ECUByte r = ECUReadResponse();

		if (r.has_value())
		{
			result.push_back(r.value());
		}
	}

	return result;
}

bool ECUMonomotronic::ECUWriteSequential(const std::vector<uint8_t> &data)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!ECUWriteResponse(data[i]))
			return false;
	}

	return true;
}

std::optional<ECUmmpacket> ECUMonomotronic::ECUReadPacket()
{
	ECUmmpacket result;

	{
		ECUByte size = ECUReadResponse();

		if (size.has_value())
		{
			result.size = size.value();
		}
		else
		{
			return std::nullopt;
		}
	}

	{
		ECUByte counter = ECUReadResponse();

		if (counter.has_value())
		{
			result.counter = counter.value();
		}
		else
		{
			return std::nullopt;
		}
	}

	{
		ECUByte frameid = ECUReadResponse();

		if (frameid.has_value())
		{
			result.frametypeid = frameid.value();
		}
		else
		{
			return std::nullopt;
		}
	}

	if (result.size > 3)
	{
		result.data = ECUReadSequential(result.size - 3);

		if (result.data.size() != (result.size - 3))
		{
			return std::nullopt;
		}
	}

	{
		ECUByte end = ECURead();

		if (end.has_value())
		{

		}
		else
		{
			return std::nullopt;
		}
	}

	return result;
}

bool ECUMonomotronic::ECUWritePacket(uint8_t frameid, const std::vector<uint8_t> &data)
{
	int packetSize = 3 + data.size();

	if (!ECUWriteResponse(packetSize))
	{
		return false;
	}

	if (!ECUWriteResponse(++ECUPacketCounter))
	{
		return false;
	}

	if (!ECUWriteResponse(frameid))
	{
		return false;
	}

	if (data.size() > 0)
	{
		ECUWriteSequential(data);
	}

	if (!ECUWrite(0x03))
	{
		return false;
	}
	
	return true;
}

void ECUMonomotronic::init()
{
	ECUOpenThread();
}

void ECUMonomotronic::forcestop()
{
	sp.close();

	std::cout << "Err " << ECUThreadErr << std::endl;
	printlogging();
}

void ECUMonomotronic::stop()
{
	KeepECUConnectionAlive = false;
	ECUThreadExit = true;
}

void ECUMonomotronic::debugTofile()
{
	if (!ECUThreadRunning)
	{
		std::fstream fs("ECUSerial.log", std::ios::out | std::ios::trunc);
		fprintlogging(fs);
		fs.flush();
	}
}

ECUMonomotronic::ECUMonomotronic(const char *port, bool enableLogging) noexcept : sp(port)
{
	enableLog = enableLogging;
	ECUThreadExit = false;
	ECUThreadRunning = false;
	ECUInited = false;
	KeepECUConnectionAlive = true;
	ECUThreadState = 0;
	ECUThreadErr = 0;
	ECUPacketCounter = 0;
	ECUNewCommandAvailable = false;
	ECUCommandResultAvailable = false;
	ECUThreadCanAcceptCommands = false;

	bytesLogging.reserve(2048);

	debug_file = nullptr;
	debug_line = 0;
}

ECUMonomotronic::~ECUMonomotronic()
{
	if (ECUThreadRunning)
	{
		ECUThreadExit = true;
	}

	if (ECUThreadObj.joinable())
		ECUThreadObj.join();
}
