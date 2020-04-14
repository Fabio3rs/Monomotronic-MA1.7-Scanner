#pragma once
#ifndef ESP32MONOMOTRONIC_h
#define ESP32MONOMOTRONIC_h

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

#include <mutex>
#include <atomic>
#include <cstdint>
#include <vector>
#include <atomic>
#include <deque>
#include <thread>
#include "Arduino.h"

class nulloptional
{

};

static nulloptional nullopt;

template<class T>
class optional
{
	T val;
	bool hasv;

public:
	T &value() { return val; };
	bool has_value() const { return hasv; };

	operator bool() const
	{
		return hasv;
	}
	
	bool operator==(const T& val2)
	{
		if (hasv)
			return val == val2;
		
		return false;
	}

	optional &operator=(const T &v)
	{
		val = v;
		hasv = true;

		return *this;
	}

	optional &operator=(const T &&v)
	{
		val = std::move(v);
		hasv = true;

		return *this;
	}

	optional(const optional&) = default;
	optional(optional&&) = default;
	optional(T &&v) : hasv(true), val(std::move(v)) {   }
	optional(const T &v) : hasv(true), val(v) {   }
	optional() : hasv(false) {   }
	optional(const nulloptional &n) : hasv(false) {   }
};

typedef optional<uint8_t> ECUByte;

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

class ESP32Monomotronic
{
	bool inited;
	bool ECUInited;
	std::atomic<int>							taskState;
	std::atomic<int>							ECUThreadErr;
	std::atomic<bool>							ECUConnected;
	std::atomic<bool>							baudEchoOK;
	std::atomic<bool>							initPacketsOk;
	std::atomic<int>							debug_line;


	std::atomic<bool>							readSensors;
  
	std::mutex									requestCommandMutex;
	struct
	{
		uint8_t frameid;
		std::vector<uint8_t> data;
	} newCommandFrame;
	std::atomic<bool>							newCommandAvailable;


	std::mutex									getResultCommandMutex;
	std::deque<ECUmmpacket>						lastCommandPackets;
	bool										newCommandPacketsAvailable;

	int ECUPacketCounter;

	TaskHandle_t								Task1;

	static void									updatePacketCounter(ESP32Monomotronic &mm, const ECUmmpacket &p);

	std::deque<ECUmmpacket>						initPackets;
	ECUByte										ECURead(int timeout = 1000);
	bool										ECUWrite(uint8_t b);


	ECUByte										ECUReadAndResponse(int timeout = 1000);
	bool										ECUWriteWaitResponse(uint8_t b, int timeout = 1000);

	std::vector<uint8_t>						ECUReadSequential(int size, int timeout = 1000);
	bool										ECUWriteSequential(const std::vector<uint8_t> &data, int timeout = 1000);

	optional<ECUmmpacket>						ECUReadPacket(int timeout = 1000);
	bool										ECUWritePacket(uint8_t frameid, const std::vector<uint8_t> &data = std::vector<uint8_t>(), int timeout = 1000);

	optional<std::deque<ECUmmpacket>>			ECURequestData(uint8_t frameid, uint8_t eECUFrameID, const std::vector<uint8_t> &data = std::vector<uint8_t>(), int timeout = 1000);

	void										debug_regiter_err(const char *file, int line)
	{
		debug_line = line;
	}

	static void									commThread(void *mm);
public:
	int											getThreadErrorCode() const
	{
		return ECUThreadErr;
	}
 
	int                     					getThreadTaskState() const
	{
		return taskState;
	}

	bool										isECUConnected() const
	{
		return ECUConnected;
	}
 
	bool                    					isEchoBytePresent() const
	{
		return baudEchoOK;
	}
	
	int											getDebugLine() const
	{
		return debug_line;
	}

	int											requestCommand(uint8_t frameid, const std::vector<uint8_t> data)
	{
		{
			std::lock_guard<std::mutex> lck(requestCommandMutex);
			newCommandFrame.frameid = frameid;
			newCommandFrame.data = std::move(data);
		}
		newCommandAvailable = true;

		return 0;
	}

	optional<std::deque<ECUmmpacket>>			getLastCommandPackets()
	{
		std::lock_guard<std::mutex> lck(getResultCommandMutex);
		
		if (newCommandPacketsAvailable)
		{
			return std::move(lastCommandPackets);
		}

		return nullopt;
	}
	
	const std::deque<ECUmmpacket>*				getInitPackets() const
	{
		if (initPacketsOk)
		{
			return &initPackets;
		}
		
		return nullptr;
	}

	eTaskState getThreadState()
	{
		if (inited)
			return eTaskGetState(Task1);
		return eDeleted;
	}

	// Source: http://www.nailed-barnacle.co.uk/coupe/startrek/startrek.html
	enum ECU_FRAMES_ID {
		ECU_DATA_MEMORY_READ = 0x01, ECU_REQ_ACTUATOR = 0x04, ECU_CLEAR_ERRORS_CODE = 0x05, ECU_REQ_DIAGNOSIS_END = 0x06, ECU_READ_ERRORS_CODE = 0x07, ECU_ACK_CODE = 0x09, ECU_NOT_ACK_CODE = 0x0A,
		ECU_INIT_STRING = 0xF6, ECU_REQUEST_ADC_CODE = 0xFB, ECU_ERROR_DATA_CODE = 0xFC, ECU_READ_DATA_CODE = 0xFE
	};


	bool init();

	ESP32Monomotronic();
};

#endif
