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

	for (int i = 0; i < 1000; i++)
	{
		if (Serial2.available() > 0)
		{
			int code = Serial2.read();
			return true;
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
		delay(7);
		uint8_t v = val.value();
		ECUWrite(~v);
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

optional<std::deque<ECUmmpacket>> ESP32Monomotronic::ECURequestData(uint8_t frameid, uint8_t eECUFrameID, const std::vector<uint8_t> &data, int timeout)
{
	if (ECUWritePacket(frameid, data, timeout))
	{
		bool canret = false;
		std::deque<ECUmmpacket> result;

		do
		{
			optional<ECUmmpacket> ecup = ECUReadPacket(timeout);
			if (ecup.has_value())
			{
				if (ecup.value().frametypeid == ECU_ACK_CODE)
				{
					canret = true;
				}
				else
				{
					ECUWritePacket(ECU_ACK_CODE);
				}

				result.push_back(std::move(ecup.value()));
			}
			else
			{
				if (result.size() == 0)
					return nullopt;

				return std::move(result);
			}
		} while (!canret);

		return std::move(result);
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
			
			delay(2000);
		}
		
		if (connect)
		{
			mm.taskState = 0;

			uint8_t key1 = 0, key2 = 0, key3 = 0, key4 = 0;

			baudInit();
			int state = 0;

			mm.taskState = 1;
			for (int i = 0; i < 30 && state != 5; i++)
			{
				char b = 0;
				ECUByte code = mm.ECURead(200);
				
				if (code.has_value())
				{
					if (tries > 0)
					{
						CLog::l().logwrite(std::string("Code is ") + String(code.value(), HEX).c_str());
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

			if (state == 5)
			{
				delay(22);
				mm.ECURead(20);
				delay(22);
				if (!mm.ECUWrite(~key2))
				{
					mm.debug_regiter_err(__FILE__, __LINE__);
					mm.ECUThreadErr = 1;
					nError = true;
				}
				else
				{
					mm.taskState = 2;
					mm.ECUConnected = true;
				}
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
									Serial.println("Packet size " + String(mm.initPackets.back().size, HEX));
									Serial.println("Packet counter " + String(mm.initPackets.back().counter, HEX));
									Serial.println("Packet frametypeid " + String(mm.initPackets.back().frametypeid, HEX));
									
									CLog::l().logwrite(std::string("Packet size ") + String(mm.initPackets.back().size, HEX).c_str());
									CLog::l().logwrite(std::string("Packet counter ") + String(mm.initPackets.back().counter, HEX).c_str());
									CLog::l().logwrite(std::string("Packet frametype ") + String(mm.initPackets.back().frametypeid, HEX).c_str());
									
									for (auto &a : mm.initPackets.back().data)
									{
										Serial.println("Packet data " + String(a, HEX));
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
				while (isConnected)
				{
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
			3,           /* priority of the task */
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
}
