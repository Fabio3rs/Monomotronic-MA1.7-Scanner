#include "SerialPort.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/serial.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

SerialPort::SerialPort(const std::string &portName, speed_t baudRate) noexcept
    : fd(-1), connected(false) {
    // Removed O_SYNC - doesn't help USB timing (only for file I/O persistence)
    fd = open(portName.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Error opening serial port");
        return;
    }

    // Detect device type for optimizations
    if (portName.starts_with("/dev/pts/")) {
        ispty = true;
    } else if (portName.find("ttyUSB") != std::string::npos ||
               portName.find("ttyACM") != std::string::npos) {
        is_usb = true;
    }

    printf("Opened port %s (ispty=%d, is_usb=%d)\n", portName.c_str(),
           ispty ? 1 : 0, is_usb ? 1 : 0);

    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        ::close(fd);
        return;
    }

    // Use cfmakeraw() for complete raw mode (disables all input translations)
    // This prevents CR/LF mapping, strip bits, etc. that can corrupt binary ECU
    // data
    cfmakeraw(&tty);

    // Set baud rate
    cfsetispeed(&tty, baudRate);
    cfsetospeed(&tty, baudRate);

    // Configure serial parameters
    tty.c_cflag |= (CLOCAL | CREAD);   // Enable receiver, ignore modem control
    tty.c_cflag &= ~CRTSCTS;           // No hardware flow control
    tty.c_cflag &= ~(PARENB | PARODD); // No parity
    tty.c_cflag &= ~CSTOPB;            // 1 stop bit
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 data bits

    // Allow break signal processing (needed for 5-baud initialization)
    tty.c_iflag &= static_cast<tcflag_t>(~IGNBRK);

    // CRITICAL FIX: VMIN=1, VTIME=0 for byte-by-byte reads
    // - VMIN=1: Block until at least 1 byte is available
    // - VTIME=0: No inter-byte timeout (let poll() handle millisecond timeouts)
    // This eliminates race conditions with USB buffering
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        ::close(fd);
        return;
    }

    // USB low-latency mode: Reduce FTDI latency timer from 16ms to 1ms
    // NOTE: This has limitations:
    // - FTDI: Works, but may require CAP_SYS_ADMIN privilege
    // - CH340/CH341: TIOCGSERIAL often not supported (will warn but continue)
    // - Better alternative: Set sysfs latency_timer via udev rule:
    //   SUBSYSTEM=="usb-serial", DRIVER=="ftdi_sio", ATTR{latency_timer}="1"
    if (is_usb) {
#ifdef __linux__
        struct serial_struct serial_settings;
        if (ioctl(fd, TIOCGSERIAL, &serial_settings) == 0) {
            serial_settings.flags |= ASYNC_LOW_LATENCY;
            if (ioctl(fd, TIOCSSERIAL, &serial_settings) < 0) {
                std::cerr << "Warning: Failed to set ASYNC_LOW_LATENCY: "
                          << strerror(errno) << std::endl;
                std::cerr << "  (May require CAP_SYS_ADMIN or udev rule)"
                          << std::endl;
            } else {
                std::cout << "USB low-latency mode enabled (FTDI: 16ms -> 1ms)"
                          << std::endl;
            }
        } else {
            // CH340/CH341 often fail here - not an error
            std::cerr << "Info: TIOCGSERIAL not supported (CH340/CH341?)"
                      << std::endl;
            std::cerr << "  USB latency will remain at default (~16ms)"
                      << std::endl;
        }
#endif
    }

    connected = true;
}

SerialPort::~SerialPort() {
    if (connected)
        close();
}

int SerialPort::available() const {
    int n = 0;
    ioctl(fd, FIONREAD, &n);
    return n;
}

bool SerialPort::isConnected() const noexcept { return connected; }

void SerialPort::close() {
    if (connected) {
        ::close(fd);
        connected = false;
        fd = -1;
    }
}

bool SerialPort::setBreak() {
    if (ispty) {
        // Em PTY, emule BREAK escrevendo o triplete PARMRK (FF 00 00)
        const uint8_t breakSeq[3] = {0xFF, 0x00, 0x00};
        writeUSerialPort((void *)breakSeq, 3);
        return true;
    }

    if (fd < 0)
        return false;
    return (ioctl(fd, TIOCSBRK) == 0);
}

bool SerialPort::clearBreak() {
    if (ispty) {
        // Em PTY, emule CLEAR BREAK escrevendo o triplete PARMRK (FF 00 FF)
        const uint8_t clearBreakSeq[3] = {0xFF, 0x00, 0xFF};
        writeUSerialPort((void *)clearBreakSeq, 3);
        return true;
    }
    if (fd < 0)
        return false;
    return (ioctl(fd, TIOCCBRK) == 0);
}

bool SerialPort::scBreak(bool breakState) {
    return breakState ? setBreak() : clearBreak();
}

bool SerialPort::writeByte(uint8_t b) {
    return writeUSerialPort(&b, sizeof(b));
}

int SerialPort::readSerialPort(char *buffer, unsigned int buf_size) {
    if (fd < 0)
        return -1;
    ssize_t rd = ::read(fd, buffer, buf_size);
    return (rd < 0) ? 0 : static_cast<int>(rd);
}

int SerialPort::readUSerialPort(void *buffer, unsigned int buf_size) {
    if (fd < 0)
        return -1;
    ssize_t rd = ::read(fd, buffer, buf_size);
    return (rd < 0) ? 0 : static_cast<int>(rd);
}

std::optional<uint8_t> SerialPort::fastByteReadUSerialPort(int timeout_ms) {
    if (fd < 0)
        return std::nullopt;

    if (timeout_ms > 0) {
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLIN;
        int rv = ::poll(&pfd, 1, timeout_ms);
        if (rv <= 0 || (pfd.revents & POLLIN) == 0) {
            return std::nullopt;
        }
    }

    uint8_t byte;
    ssize_t rd = ::read(fd, &byte, 1);
    if (rd == 1)
        return byte;
    return std::nullopt;
}

bool SerialPort::writeSerialPort(char *buffer, unsigned int buf_size) {
    if (fd < 0)
        return false;
    ssize_t written = ::write(fd, buffer, buf_size);
    return (written == static_cast<ssize_t>(buf_size));
}

bool SerialPort::writeUSerialPort(void *buffer, unsigned int buf_size) {
    if (fd < 0)
        return false;
    ssize_t written = ::write(fd, buffer, buf_size);
    drainOutput();
    return (written == static_cast<ssize_t>(buf_size));
}

void SerialPort::flushInput() {
    if (fd >= 0) {
        tcflush(fd, TCIFLUSH);
    }
}

void SerialPort::flushOutput() {
    if (fd >= 0) {
        tcflush(fd, TCOFLUSH);
    }
}

void SerialPort::flushBoth() {
    if (fd >= 0) {
        tcflush(fd, TCIOFLUSH);
    }
}

void SerialPort::drainOutput() {
    if (fd >= 0) {
        tcdrain(fd);
    }
}
