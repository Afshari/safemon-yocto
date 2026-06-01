#include "can_reader.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/time.h>
#include <cerrno>

CanReader::CanReader(const std::string& interface)
    : interface_(interface), socket_fd_(-1) {}

CanReader::~CanReader() {
    close();
}

bool CanReader::open() {
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating CAN socket" << std::endl;
        return false;
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, interface_.c_str(), IFNAMSIZ - 1);
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Error getting interface index" << std::endl;
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding CAN socket" << std::endl;
        return false;
    }

    struct timeval tv;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::cout << "CAN socket opened on " << interface_ << std::endl;
    return true;
}

void CanReader::close() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool CanReader::read(CanFrame& frame) {
    struct can_frame raw;
    ssize_t nbytes = ::read(socket_fd_, &raw, sizeof(raw));
    if (nbytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;  // timeout - not an error
        }
        std::cerr << "Error reading CAN frame" << std::endl;
        return false;
    }

    frame.id  = raw.can_id & CAN_EFF_MASK;
    frame.len = raw.can_dlc;
    std::memcpy(frame.data, raw.data, frame.len);
    return true;
}