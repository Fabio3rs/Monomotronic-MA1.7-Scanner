#pragma once
#ifndef ECUMONOMOTRONIC_H
#define ECUMONOMOTRONIC_H

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

#include "SerialPort.h"
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <array>
#include <deque>
#include <fstream>

typedef std::optional<uint8_t> ECUByte;

struct ECUmmpacket
{
	uint8_t size;
	uint8_t counter;
	uint8_t frametypeid;

	std::vector<uint8_t> data;

	uint8_t end;

	ECUmmpacket &operator=(ECUmmpacket&&) = default;
	ECUmmpacket &operator=(const ECUmmpacket&) = default;
	ECUmmpacket(ECUmmpacket&&) = default;
	ECUmmpacket(const ECUmmpacket&) = default;

	ECUmmpacket() : size(3), counter(0), frametypeid(0), end(0x03)
	{

	}
};

class ECUMonomotronic
{
	SerialPort				sp;
	std::thread				ECUThreadObj;
	std::atomic<bool>		ECUThreadRunning;
	std::atomic<bool>		ECUThreadExit;
	std::atomic<int>		ECUThreadState;
	std::atomic<int>		ECUThreadErr;
	std::atomic<bool>		ECUInited;
	std::atomic<bool>		KeepECUConnectionAlive;
	uint8_t					ECUPacketCounter;

	bool					enableLog;

	//						send commands variables
	std::atomic<bool>		ECUThreadCanAcceptCommands;
	std::mutex				ECUNewCommandMutex;
	ECUmmpacket				ECUNewCommandTemp;
	std::atomic<bool>		ECUNewCommandAvailable;

	std::atomic<bool>		ECUCommandResultAvailable;
	ECUmmpacket				ECUResponse;

	std::deque<ECUmmpacket> initPackets;

	struct blogging {
		std::chrono::high_resolution_clock::time_point time;
		int act;
		int byte;

		blogging(int a, int b)
		{
			time = std::chrono::high_resolution_clock::now();
			act = a;
			byte = b;
		}
	};

	std::vector<blogging> bytesLogging;

	void addbytelog(const blogging &b)
	{
		bytesLogging.push_back(b);
	}

	void ECUOpenThread();
	void sendInitSequence();
	static void updatePacketCounter(ECUMonomotronic &mm, const ECUmmpacket &p);
	static void ECUThreadFun(ECUMonomotronic &mm);

	bool							ECUWrite(uint8_t b);
	ECUByte							ECURead();
	ECUByte							ECUReadResponse();
	bool							ECUWriteResponse(uint8_t b);
	std::vector<uint8_t>			ECUReadSequential(int size);
	bool							ECUWriteSequential(const std::vector<uint8_t> &data);
	std::optional<ECUmmpacket>		ECUReadPacket();
	bool							ECUWritePacket(uint8_t frameid, const std::vector<uint8_t> &data = std::vector<uint8_t>());

	void							printlogging();
	void							fprintlogging(std::fstream &fs);

	const char						*debug_file;
	int								debug_line;

	void							debug_regiter_err(const char *file, int line);

	std::optional<std::deque<ECUmmpacket>>		ECURequestData(uint8_t frameid, uint8_t eECUFrameID, const std::vector<uint8_t> &data = std::vector<uint8_t>());

public:

	// Init received packages/ECU identification
	const std::deque<ECUmmpacket>	&getinitPackets() const { return initPackets; };

	// Custom commands
	// See ECU_FRAMES_ID
	std::optional<ECUmmpacket>		getECUResponse();
	bool							sendECURequest(uint8_t frameid, const std::vector<uint8_t> &data = std::vector<uint8_t>());


	// Wrapper to commands
	std::optional<std::deque<ECUmmpacket>>		ECUReadErrors();
	std::optional<std::deque<ECUmmpacket>>		ECUReadSensor(uint8_t sensorID);
	std::optional<ECUmmpacket>					ECUCleanErrors();

	// Source: http://www.nailed-barnacle.co.uk/coupe/startrek/startrek.html
	enum ECU_FRAMES_ID { ECU_DATA_MEMORY_READ = 0x01, ECU_REQ_ACTUATOR = 0x04, ECU_CLEAR_ERRORS_CODE = 0x05, ECU_REQ_DIAGNOSIS_END = 0x06, ECU_READ_ERRORS_CODE = 0x07, ECU_ACK_CODE = 0x09, ECU_NOT_ACK_CODE = 0x0A,
						ECU_INIT_STRING = 0xF6, ECU_REQUEST_ADC_CODE = 0xFB, ECU_ERROR_DATA_CODE = 0xFC, ECU_READ_DATA_CODE = 0xFE};

	// Error codes to description
	static std::string				errorPacketToString(const ECUmmpacket &p, bool &present);

	bool canAcceptCommands() const { return ECUThreadCanAcceptCommands; }
	bool isThreadRunning() const { return ECUThreadRunning; }
	bool portIsOpen() const noexcept { return sp.isConnected(); };

	// Interface controls
	void init();
	void forcestop();
	void stop();

	// Debugging
	void debugTofile();
	void purgeSerial();
	
	ECUMonomotronic(const char *port, bool enableLogging = true) noexcept;
	~ECUMonomotronic();
};


#endif

