#include "SerialPort.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/serial.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace {
void ReportUnsupportedBaudRate(const std::string &portName, uint32_t baudRate,
                               bool isStandardBaud) {
    std::cerr << "Failed to configure port " << portName << " with baud "
              << baudRate << ". ";
    if (isStandardBaud) {
        std::cerr << "The operating system or serial driver rejected this "
                     "standard baud rate.";
    } else {
        std::cerr << "The serial driver or USB adapter does not support this "
                     "arbitrary baud rate.";
    }
    std::cerr << std::endl;
}

#ifdef __linux__
struct termios2 {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[NCCS];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

#ifndef BOTHER
constexpr tcflag_t kBOTHER = 0x00001000;
#else
constexpr tcflag_t kBOTHER = BOTHER;
#endif

#ifndef CBAUD
constexpr tcflag_t kCBAUD = 0x0000100f;
#else
constexpr tcflag_t kCBAUD = CBAUD;
#endif

#ifndef TCGETS2
#define TCGETS2 _IOR('T', 0x2A, struct termios2)
#endif

#ifndef TCSETS2
#define TCSETS2 _IOW('T', 0x2B, struct termios2)
#endif
#endif

std::optional<speed_t> MapBaudRateToSpeedT(uint32_t baudRate) noexcept {
    switch (baudRate) {
    case 50:
        return B50;
    case 75:
        return B75;
    case 110:
        return B110;
    case 134:
        return B134;
    case 150:
        return B150;
    case 200:
        return B200;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 1800:
        return B1800;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
#ifdef B19200
    case 19200:
        return B19200;
#endif
#ifdef B38400
    case 38400:
        return B38400;
#endif
#ifdef B57600
    case 57600:
        return B57600;
#endif
#ifdef B115200
    case 115200:
        return B115200;
#endif
#ifdef B230400
    case 230400:
        return B230400;
#endif
#ifdef B460800
    case 460800:
        return B460800;
#endif
#ifdef B500000
    case 500000:
        return B500000;
#endif
#ifdef B576000
    case 576000:
        return B576000;
#endif
#ifdef B921600
    case 921600:
        return B921600;
#endif
#ifdef B1000000
    case 1000000:
        return B1000000;
#endif
#ifdef B1152000
    case 1152000:
        return B1152000;
#endif
#ifdef B1500000
    case 1500000:
        return B1500000;
#endif
#ifdef B2000000
    case 2000000:
        return B2000000;
#endif
#ifdef B2500000
    case 2500000:
        return B2500000;
#endif
#ifdef B3000000
    case 3000000:
        return B3000000;
#endif
#ifdef B3500000
    case 3500000:
        return B3500000;
#endif
#ifdef B4000000
    case 4000000:
        return B4000000;
#endif
    default:
        return std::nullopt;
    }
}

bool SetBaudRate(int fd, termios &tty, uint32_t baudRate) noexcept {
    if (const std::optional<speed_t> speed = MapBaudRateToSpeedT(baudRate);
        speed.has_value()) {
        return cfsetispeed(&tty, speed.value()) == 0 &&
               cfsetospeed(&tty, speed.value()) == 0;
    }

#ifdef __linux__
    struct termios2 tty2 {};
    if (ioctl(fd, TCGETS2, &tty2) != 0) {
        return false;
    }

    tty2.c_cflag &= ~kCBAUD;
    tty2.c_cflag |= kBOTHER;
    tty2.c_ispeed = baudRate;
    tty2.c_ospeed = baudRate;

    return ioctl(fd, TCSETS2, &tty2) == 0;
#else
    (void)fd;
    (void)tty;
    (void)baudRate;
    return false;
#endif
}
} // namespace

SerialPort::SerialPort(const std::string &portName, uint32_t baudRate) noexcept
    : fd(-1), connected(false), configured_baud_rate(baudRate) {
    termios tty{};

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

    std::cout << "Opening serial port: " << portName;
    if (is_usb) {
        std::cout << " (USB serial adapter)";
    } else if (ispty) {
        std::cout << " (PTY)";
    }
    std::cout << std::endl;

    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        ::close(fd);
        return;
    }

    // Use cfmakeraw() for complete raw mode (disables all input translations)
    // This prevents CR/LF mapping, strip bits, etc. that can corrupt binary ECU
    // data
    cfmakeraw(&tty);

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

    // Apply standard baud rates via termios when available.
    const bool uses_standard_baud = MapBaudRateToSpeedT(baudRate).has_value();
    if (uses_standard_baud && !SetBaudRate(fd, tty, baudRate)) {
        ReportUnsupportedBaudRate(portName, baudRate, true);
        ::close(fd);
        fd = -1;
        return;
    }

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        ::close(fd);
        return;
    }

    // Apply arbitrary baud rates after tcsetattr() so BOTHER is not clobbered
    // by a later termios write.
    if (!uses_standard_baud && !SetBaudRate(fd, tty, baudRate)) {
        ReportUnsupportedBaudRate(portName, baudRate, false);
        ::close(fd);
        fd = -1;
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
