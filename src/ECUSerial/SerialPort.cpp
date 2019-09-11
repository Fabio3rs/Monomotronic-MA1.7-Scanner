#include "SerialPort.h"

bool SerialPort::scBreak(bool breakState)
{
	if (breakState)
		return SetCommBreak(handler);

	return ClearCommBreak(handler);
}
bool SerialPort::setBreak()
{
	return SetCommBreak(handler);
}
bool SerialPort::clearBreak()
{
	return ClearCommBreak(handler);
}
COMSTAT SerialPort::getStat()
{
	ClearCommError(handler, &errors, &status);
	return status;
}

SerialPort::SerialPort(const char *portName) noexcept
{
	connected = false;

	handler = CreateFileA(portName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (handler == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			printf("ERROR: Handle was not attached. Reason: %s not available\n", portName);
		}
		else
		{
			printf("ERROR!!!");
		}
	}
	else {
		DCB dcbSerialParameters = { 0 };

		if (!GetCommState(handler, &dcbSerialParameters)) {
			printf("failed to get current serial parameters");
		}
		else {
			dcbSerialParameters.BaudRate = 4800;
			dcbSerialParameters.ByteSize = 8;
			dcbSerialParameters.StopBits = ONESTOPBIT;
			dcbSerialParameters.Parity = NOPARITY;
			dcbSerialParameters.fOutxCtsFlow = FALSE;
			dcbSerialParameters.fOutxDsrFlow = FALSE;
			dcbSerialParameters.fRtsControl = RTS_CONTROL_DISABLE;
			dcbSerialParameters.fDtrControl = DTR_CONTROL_DISABLE;

			if (!SetCommState(handler, &dcbSerialParameters))
			{
				printf("ALERT: could not set Serial port parameters\n");
			}
			else {
				connected = true;
				PurgeComm(handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
				Sleep(10);
			}

			if (!SetupComm(handler, 1024, 0))
				printf("SetupComm(handler, 1024, 0) FAILED\n");
		}
	}
}

SerialPort::~SerialPort()
{
	if (connected)
	{
		connected = false;
		CloseHandle(handler);
	}
}

void SerialPort::close()
{
	if (this->connected)
	{
		this->connected = false;
		CloseHandle(this->handler);
	}
}

int SerialPort::readSerialPort(char *buffer, unsigned int buf_size)
{
	DWORD bytesRead = 0;
	unsigned int toRead;

	ClearCommError(handler, &errors, &status);

	if (this->status.cbInQue > 0)
	{
		if (this->status.cbInQue > buf_size)
		{
			toRead = buf_size;
		}
		else toRead = status.cbInQue;
	}

	if (ReadFile(handler, buffer, toRead, &bytesRead, NULL)) return bytesRead;

	return 0;
}

int SerialPort::readUSerialPort(void *buffer, unsigned int buf_size)
{
	DWORD bytesRead = 0;
	unsigned int toRead;

	ClearCommError(handler, &errors, &status);

	if (this->status.cbInQue > 0)
	{
		if (this->status.cbInQue > buf_size)
		{
			toRead = buf_size;
		}
		else toRead = status.cbInQue;
	}

	if (ReadFile(handler, buffer, toRead, &bytesRead, NULL)) return bytesRead;

	return 0;
}

std::optional<uint8_t> SerialPort::fastByteReadUSerialPort()
{
	DWORD bytesRead = 0;
	uint8_t byte = 0;

	if (ReadFile(handler, &byte, sizeof(byte), &bytesRead, NULL)) return std::make_optional<uint8_t>(byte);

	ClearCommError(handler, &errors, &status);
	return std::nullopt;
}

bool SerialPort::writeSerialPort(char *buffer, unsigned int buf_size)
{
	DWORD bytesSend;

	if (!WriteFile(handler, (void*)buffer, buf_size, &bytesSend, 0))
	{
		ClearCommError(handler, &errors, &status);
		return false;
	}
	
	return true;
}

bool SerialPort::writeUSerialPort(void *buffer, unsigned int buf_size)
{
	DWORD bytesSend;

	if (!WriteFile(handler, buffer, buf_size, &bytesSend, 0))
	{
		ClearCommError(handler, &errors, &status);
		return false;
	}
	
	return true;
}

bool SerialPort::isConnected() const noexcept
{
	return this->connected;
}