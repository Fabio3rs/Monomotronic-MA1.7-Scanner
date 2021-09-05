#ifndef SERIALPORT_H
#define SERIALPORT_H
#define _CRT_SECURE_NO_WARNINGS
#include <optional>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

class SerialPort {
  private:
    HANDLE handler;
    bool connected;
    COMSTAT status;
    DWORD errors;

  public:
    bool scBreak(bool breakState);
    bool setBreak();
    bool clearBreak();

    COMSTAT getStat();

    SerialPort(const char *portName) noexcept;
    ~SerialPort();

    void close();

    bool writeByte(uint8_t b) { return writeUSerialPort(&b, sizeof(b)); }
    int readSerialPort(char *buffer, unsigned int buf_size);
    int readUSerialPort(void *buffer, unsigned int buf_size);
    std::optional<uint8_t> fastByteReadUSerialPort();
    bool writeSerialPort(char *buffer, unsigned int buf_size);
    bool writeUSerialPort(void *buffer, unsigned int buf_size);
    bool isConnected() const noexcept;
};

#endif // SERIALPORT_H