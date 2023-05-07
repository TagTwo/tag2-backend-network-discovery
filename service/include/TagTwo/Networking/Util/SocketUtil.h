//
// Created by per on 4/7/23.
//

#ifndef TAGTWO_STREAMING_SOCKETUTIL_H
#define TAGTWO_STREAMING_SOCKETUTIL_H
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

namespace TagTwo::Networking::Util {

    bool checkTCPEndpoint(const std::string& host, int port, int timeout_ms = 1000) {
        addrinfo hints, *res, *p;
        int sockfd;
        bool connected = false;

        // Prepare hints for getaddrinfo
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        std::string port_str = std::to_string(port);

        // Resolve hostname
        int ret = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
        if (ret != 0) {
            return false; // Failed to resolve hostname
        }

        // Try connecting to each address returned by getaddrinfo
        for (p = res; p != nullptr && !connected; p = p->ai_next) {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd < 0) {
                continue;
            }

            // Set socket to non-blocking
            int flags = fcntl(sockfd, F_GETFL, 0);
            fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

            // Connect to server (non-blocking)
            ret = connect(sockfd, p->ai_addr, p->ai_addrlen);
            if (ret < 0 && errno != EINPROGRESS) {
                close(sockfd);
                continue;
            }

            // Use select to wait for connection or timeout (if specified)
            if (timeout_ms >= 0) {
                fd_set fdset;
                struct timeval tv;
                FD_ZERO(&fdset);
                FD_SET(sockfd, &fdset);
                tv.tv_sec = timeout_ms / 1000;
                tv.tv_usec = (timeout_ms % 1000) * 1000;

                ret = select(sockfd + 1, NULL, &fdset, NULL, &tv);
                if (ret <= 0) {
                    close(sockfd);
                    continue; // Timeout or select error
                }
            }

            // Check if socket is connected
            int sock_error;
            socklen_t len = sizeof(sock_error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sock_error, &len);
            if (sock_error) {
                close(sockfd);
                continue; // Connection failed
            }

            // Connection succeeded
            connected = true;
            close(sockfd);
        }

        freeaddrinfo(res);
        return connected;
    }

}

#endif //TAGTWO_STREAMING_SOCKETUTIL_H
