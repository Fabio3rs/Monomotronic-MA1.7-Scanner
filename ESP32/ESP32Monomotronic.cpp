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

#include "ESP32Monomotronic.h"
#include "Log.h"
#include <map>

template<int n>
void sendInit(int port1, int port2, std::array<int, n> v)
{
	for (int i = 0; i < v.size(); i++)
	{
		digitalWrite(port1, v[i]);
		digitalWrite(port2, v[i]);
		delay(200);
	}
}

void baudInit()
{
	digitalWrite(0, HIGH);
	digitalWrite(2, HIGH);
	delay(200);
	sendInit<8>(0, 2, std::array<int, 8>{HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH, HIGH}); // 0x10
	digitalWrite(0, LOW);
	digitalWrite(2, LOW);
}

void ESP32Monomotronic::updatePacketCounter(ESP32Monomotronic &mm, const ECUmmpacket &p)
{
	if (p.counter > mm.ECUPacketCounter)
		mm.ECUPacketCounter = p.counter;
}

ECUByte ESP32Monomotronic::ECURead(int timeout)
{
	for (int i = 0; i < timeout; i++)
	{
		if (Serial2.available() > 0)
		{
			uint8_t code = Serial2.read() & 0xFF;
			return code;
		}
		delay(1);
	}

	return ECUByte();
}

bool ESP32Monomotronic::ECUWrite(uint8_t b)
{
	Serial2.write(b);
    //Serial2.flush();

	for (int i = 0; i < 1000; i++)
	{
		if (Serial2.available() > 0)
		{
			int code = Serial2.read();

            if (code != b)
            {
                Serial.print("Differnt " + String(code, HEX) + "  " + String(b, HEX));
            }
            
			return code == b;
		}
		delay(1);
	}

	return false;
}

ECUByte ESP32Monomotronic::ECUReadAndResponse(int timeout)
{
	ECUByte val = ECURead(timeout);

	if (val.has_value())
	{
		uint8_t v = val.value();
		ECUWrite(~v);
        delay(0);
		return val;
	}

	return ECUByte();
}

bool ESP32Monomotronic::ECUWriteWaitResponse(uint8_t b, int timeout)
{
	if (ECUWrite(b))
	{
		ECUByte val = ECURead(timeout);

		if (val.has_value())
		{
			// TODO CHECKS
			return true;
		}
	}
	return false;
}

std::vector<uint8_t> ESP32Monomotronic::ECUReadSequential(int size, int timeout)
{
	std::vector<uint8_t> result;

	result.reserve(size);

	for (int i = 0; i < size; i++)
	{
		ECUByte r = ECUReadAndResponse(timeout);

		if (r.has_value())
		{
			result.push_back(r.value());
		}
		else
		{
			return result;
		}
	}

	return result;
}


bool ESP32Monomotronic::ECUWriteSequential(const std::vector<uint8_t> &data, int timeout)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!ECUWriteWaitResponse(data[i], timeout))
			return false;
	}

	return true;
}

optional<ECUmmpacket> ESP32Monomotronic::ECUReadPacket(int timeout)
{
	ECUmmpacket result;

	{
		ECUByte size = ECUReadAndResponse(timeout);

		if (size.has_value())
		{
			result.size = size.value();
		}
		else
		{
			return nullopt;
		}
	}

	{
		ECUByte counter = ECUReadAndResponse(timeout);

		if (counter.has_value())
		{
			result.counter = counter.value();
		}
		else
		{
			return nullopt;
		}
	}

	{
		ECUByte frameid = ECUReadAndResponse(timeout);

		if (frameid.has_value())
		{
			result.frametypeid = frameid.value();
		}
		else
		{
			return nullopt;
		}
	}

	if (result.size > 3)
	{
		result.data = ECUReadSequential(result.size - 3, timeout);

		if (result.data.size() != (result.size - 3))
		{
			return nullopt;
		}
	}

	{
		ECUByte end = ECURead(timeout);

		if (end.has_value())
		{

		}
		else
		{
			return nullopt;
		}
	}

	return result;
}

bool ESP32Monomotronic::ECUWritePacket(uint8_t frameid, const std::vector<uint8_t> &data, int timeout)
{
	int packetSize = 3 + data.size();

	if (!ECUWriteWaitResponse(packetSize, timeout))
	{
		return false;
	}

	if (!ECUWriteWaitResponse(++ECUPacketCounter, timeout))
	{
		return false;
	}

	if (!ECUWriteWaitResponse(frameid, timeout))
	{
		return false;
	}

	if (data.size() > 0)
	{
		ECUWriteSequential(data, timeout);
	}

	if (!ECUWrite(0x03))
	{
		return false;
	}

	return true;
}

optional<ECUmmpacket> ESP32Monomotronic::getECUResponse()
{
	if (ECUCommandResultAvailable)
	{
		ECUCommandResultAvailable = false;
		return std::move(ECUResponse);
	}

	return nullopt;
}

bool ESP32Monomotronic::sendECURequest(uint8_t frameid, const std::vector<uint8_t> &data)
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

optional<std::deque<ECUmmpacket>> ESP32Monomotronic::ECUReadErrors()
{
	return ECURequestData(ECU_READ_ERRORS_CODE, ECU_ERROR_DATA_CODE);
}

optional<std::deque<ECUmmpacket>> ESP32Monomotronic::ECUReadSensor(uint8_t sensorID)
{
	return ECURequestData(ECU_DATA_MEMORY_READ, ECU_READ_DATA_CODE, { 0x01u, 0x00u, sensorID });
}

optional<ECUmmpacket> ESP32Monomotronic::ECUCleanErrors()
{
	/*
	TODO: Handle errors, implement timeout
	*/
	if (canAcceptCommands())
	{
		optional<ECUmmpacket>		ecuptmp;

		bool sendACK = false;
		do
		{
			sendACK = sendECURequest(ECU_CLEAR_ERRORS_CODE);
			delay(100);
		} while (!sendACK);

		do
		{
			delay(10);
			ecuptmp = getECUResponse().value();
		} while (!ecuptmp.has_value());

		if (ecuptmp.value().frametypeid == 0x03)
		{
			ECUWriteWaitResponse(0x10);
		}

		return std::move(ecuptmp);
	}

	return nullopt;
}

const char* ESP32Monomotronic::errorPacketToString(const ECUmmpacket &p, bool &present)
{
	if (p.frametypeid == ECU_ERROR_DATA_CODE)
	{
		if (p.data.size() >= 3)
		{
			// Source: http://www.fiat-tipo.ru/fpost8823.html
			static const std::map<int, const char*> errList = {
				{ 0x1A01, "Atuador de marcha lenta - Sinal ruim? Motor com do atuador de passo com defeito? Travado?"},
				{ 0x0302, "Sensor de rotacao do virabrequim | Sem sinal/motor desligado"},
				{ 0x0402, "Interruptor do atuador de marcha lenta | Curto ao GND ou VCC"},
				{ 0x0602, "Erro TPS"},
				{ 0x0A02, "Sensor de temperatura do liquido de arrefecimento"},
				{ 0x0B02, "Sensor de temperatura do ar circuito aberto ou curto para GND ou VCC" },
				{ 0x0D02, "Sonda lambda | problema sinal"},
				{ 0x3102, "Erro de correcao de mistura"},
				{ 0x0303, "Sensor de rotacao do virabrequim | Erro de sincronismo do sensor de posicao do virabrequim" },
				{ 0x3A46, "Imobilizador | Ativo" },
				{ 0xFFFF, "Erro de ECU | Defeito no computador ou selecionado incorretamente OU problema de conexao" }
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

			return "Unknown error code";
		}
	}
	return "";
}

optional<std::deque<ECUmmpacket>> ESP32Monomotronic::ECURequestData(uint8_t frameid, uint8_t eECUFrameID, const std::vector<uint8_t> &data, int timeout)
{
	if (sendECURequest(frameid, data))
	{
		std::deque<ECUmmpacket>					ecup;

		uint8_t frametypeidtmp = 0;

		do
		{
			delay(10);

			optional<ECUmmpacket>			    ecuptmp;
			do
			{
				delay(10);
				ecuptmp = getECUResponse();
			} while (!ecuptmp.has_value());

			// Copy the frame type id
			frametypeidtmp = ecuptmp.value().frametypeid;

			if (frametypeidtmp != ECU_ACK_CODE)
			{
				if (frametypeidtmp != eECUFrameID)
				{
					// Unexpected packet
					// TODO: Handle it, send NOT ACK?
					//ECUWriteResponse(ECU_NOT_ACK_CODE);
					return nullopt;
				}

				// Send "Acknowledge" packet to the ECU
				bool sendACK = false;
				do
				{
					delay(10);
					sendACK = sendECURequest(ECU_ACK_CODE);
				} while (!sendACK);
			}

			// Save the ECU's response
			ecup.push_back(std::move(ecuptmp.value()));
		} while (frametypeidtmp != ECU_ACK_CODE);

		return std::move(ecup);
	}

	return nullopt;
}

void ESP32Monomotronic::commThread(void *vpmm)
{
	ESP32Monomotronic &mm = *(ESP32Monomotronic*)vpmm;
	mm.taskState = 0;
	mm.baudEchoOK = false;

	bool connect = true;
	bool nError = false;
	bool isConnected = true;
	mm.ECUConnected = false;
	mm.initPacketsOk = false;
	mm.ECUThreadCanAcceptCommands = false;

	mm.ECUThreadErr = 0;
	bool ECUSendSyncCode = false;
  
	unsigned long lastPacketTime = 0;
	
	int tries = 0;

	while (true)
	{
		if (nError)
		{
			++tries;
			nError = false;
			mm.ECUConnected = false;
			mm.initPacketsOk = false;
			mm.ECUThreadCanAcceptCommands = false;
			
			delay(2000);
		}
		
		if (connect)
		{
			mm.taskState = 0;

			uint8_t key1 = 0, key2 = 0, key3 = 0, key4 = 0;
while (Serial2.available() > 0)
         {
            Serial2.read();
				delay(10);
          }
            Serial2.flush();

			baudInit();
			int state = 0;
            ECUSendSyncCode = false;
            
            tries = 0;
			mm.taskState = 1;
			for (int i = 0; i < 30 && state != 5; i++)
			{
				char b = 0;
				ECUByte code = mm.ECURead(200);
				
				if (code.has_value())
				{
					//if (tries > 0)
					{/*
                        Serial.print(state);
                        Serial.print("    ");
                        Serial.println(String(code.value(), HEX));*/
						//CLog::l().logwrite(std::string("Code is ") + String(code.value(), HEX).c_str());
					}
					
					switch (state)
					{
					case 0:
						if (code.value() == 0x55 && !ECUSendSyncCode)
						{
							ECUSendSyncCode = true;
							++state;
						}else if (code.value() == 0)
						{
						  mm.baudEchoOK = true;
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
				delay(1);
			}

			if (state > 1)
			{
				delay(10);
        while (Serial2.available() > 0)
         {
            Serial2.read();
            Serial2.flush();
          }
				if (!mm.ECUWrite(~key2 & 0xFF))
				{
           CLog::l().logwrite(std::string("Code is Key2 ") + String(~key2, HEX).c_str());
					mm.debug_regiter_err(__FILE__, __LINE__);
					mm.ECUThreadErr = 1;
					nError = true;
				}
				else
				{
					mm.taskState = 2;
					mm.ECUConnected = true;
				}
                Serial2.flush();
			}
			else
			{
				mm.debug_regiter_err(__FILE__, __LINE__);
				mm.ECUThreadErr = 3;
				nError = true;
				delay(100);
			}

			if (!nError)
			{
				int code = 0;
				do
				{
					optional<ECUmmpacket> p = mm.ECUReadPacket();

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
								mm.ECUConnected = false;
								
								if (mm.initPackets.size() > 0)
								{
									CLog::l().logwrite(std::string("Packet size ") + String(mm.initPackets.back().size, HEX).c_str());
									CLog::l().logwrite(std::string("Packet counter ") + String(mm.initPackets.back().counter, HEX).c_str());
									CLog::l().logwrite(std::string("Packet frametype ") + String(mm.initPackets.back().frametypeid, HEX).c_str());
									
									for (auto &a : mm.initPackets.back().data)
									{
										CLog::l().logwrite(std::string("Packet data ") + String(a, HEX).c_str());
									}
								}
								break;
							}
						}
					}
					else
					{
						mm.debug_regiter_err(__FILE__, __LINE__);
						mm.ECUThreadErr = 3;
						nError = true;
						mm.ECUConnected = false;
						break;
					}
				} while (true);

				lastPacketTime = millis();
				mm.taskState = 3;
			}


			if (!nError)
			{
				mm.initPacketsOk = true;
				mm.taskState = 4;
				mm.ECUThreadCanAcceptCommands = true;
				while (isConnected)
				{
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
							optional<ECUmmpacket> p = mm.ECUReadPacket();

							if (p.has_value())
							{
								updatePacketCounter(mm, p.value());

								mm.ECUResponse = std::move(p.value());

								lastPacketTime = millis();
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

						if (mm.ECUWaitAndReconnect)
						{
							mm.ECUWaitAndReconnect = false;
							isConnected = false;
							mm.taskState = 0;
							mm.initPacketsOk = false;
							mm.ECUThreadCanAcceptCommands = false;
							delay(5000);
							break;
						}
					}

					/*
					Keep connection alive sending ACK
					*/
					unsigned long timen = millis();

					if ((timen - lastPacketTime) > 500)
					{
						if (!mm.ECUWritePacket(ECU_ACK_CODE))
						{
							mm.debug_regiter_err(__FILE__, __LINE__);
							mm.ECUThreadErr = 4;
							nError = true;
						}
						else
						{
							int code = 0;
							optional<ECUmmpacket> p = mm.ECUReadPacket();
							updatePacketCounter(mm, p.value());
							code = p.value().frametypeid;

							if (code == ECU_ACK_CODE)
							{
								// ok
							}
							else
							{
								mm.debug_regiter_err(__FILE__, __LINE__);
								mm.ECUThreadErr = 4;
								nError = true;
								mm.ECUConnected = false;
								break;
							}
						}
						
						lastPacketTime = millis();
					}
					/*
					TODO: Manage connection, request data
					Send ACKs
					*/
					delay(50);
				}
			}
		}
	}


}

bool ESP32Monomotronic::init()
{
	if (!inited)
	{
		inited = true;
		ECUInited = false;
		taskState = 0;
		ECUPacketCounter = 0;

		xTaskCreatePinnedToCore(
			commThread,   /* Task function. */
			"Task1",     /* name of task. */
			10000,       /* Stack size of task */
			this,        /* parameter of the task */
			32,           /* priority of the task */
			&Task1,      /* Task handle to keep track of created task */
			1);          /* pin task to core 1 - ESP32 uses core 0 as default to wifi handling*/
	}
	return false;
}

ESP32Monomotronic::ESP32Monomotronic()
{
	Task1 = nullptr;
	inited = false;
	ECUInited = false;
	taskState = 0;
	ECUPacketCounter = 0;
	ECUThreadErr = 0;
	ECUThreadCanAcceptCommands = false;
	ECUNewCommandAvailable = false;
	ECUWaitAndReconnect = false;
}
