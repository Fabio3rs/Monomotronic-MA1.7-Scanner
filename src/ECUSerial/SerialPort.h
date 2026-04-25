#pragma once
#ifndef SERIALPORT_H
#define SERIALPORT_H
#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <termios.h>

class SerialPort {
  private:
    int fd;
    bool connected{false}, ispty{false}, is_usb{false};
    struct termios tty;

  public:
    SerialPort(const std::string &portName, speed_t baudRate = B4800) noexcept;
    ~SerialPort();

    int available() const;

    bool isConnected() const noexcept;
    void close();

    bool setBreak();
    bool clearBreak();
    bool scBreak(bool breakState);

    void closePort(); // alias for close()

    bool writeByte(uint8_t b);
    int readSerialPort(char *buffer, unsigned int buf_size);
    int readUSerialPort(void *buffer, unsigned int buf_size);
    std::optional<uint8_t> fastByteReadUSerialPort(int timeout_ms = 0);
    bool writeSerialPort(char *buffer, unsigned int buf_size);
    bool writeUSerialPort(void *buffer, unsigned int buf_size);

    void flushInput();  // tcflush(TCIFLUSH) - limpa RX buffer
    void flushOutput(); // tcflush(TCOFLUSH) - limpa TX buffer
    void flushBoth();   // tcflush(TCIOFLUSH) - limpa ambos
    void drainOutput(); // tcdrain() - aguarda TX completar

    bool isPty() const noexcept { return ispty; }
    bool isUSBDevice() const noexcept { return is_usb; }
};

#endif // SERIALPORT_H
