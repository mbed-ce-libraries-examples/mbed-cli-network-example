#include "network_demo_cmds.h"

#include <ns_cmdline.h>
#include <OnboardNetworkStack.h>
#include <cinttypes>

#include "mbed.h"

/*
 * Command to send a packet over UDP with the specified contents.
 *
 * This is designed to work with a command like the following on the host PC:
 * $ netcat -ul <listen port>
 */
int udp_send(int argc, char *argv[]) {
    if (argc != 4) {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }

    // Append a newline to the payload so it will show up more cleanly in netcat at the other end
    std::string payload(argv[3]);
    payload += "\n";

    // Open UDP socket
    UDPSocket udpSocket;
    auto err = udpSocket.open(&OnboardNetworkStack::get_default_instance());
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to open UDP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    // Bind to an ephemeral port (i.e. a randomly chosen port number)
    err = udpSocket.bind(0);
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to bind UDP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    // Determine dest IP and port
    uint16_t destPort = std::stoi(std::string(argv[2]));
    SocketAddress dest(argv[1], destPort);
    printf("Sending %zu bytes to %s:%" PRIu16 "\n", payload.size(), dest.get_ip_address(), dest.get_port());

    err = udpSocket.sendto(dest, payload.data(), payload.size());
    if(err < 0) {
        printf("Failed to send: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    return CMDLINE_RETCODE_SUCCESS;
}

char rxPacketBuffer[1472 + 1]; // + 1 for null terminator

/*
 * Command to begin listening for UDP data over the network.
 *
 * This is designed to be used with a netcat command like:
 * $ netcat -u -p <Tx port> <Mbed device IP> <Rx port>
 */
int udp_listen(int argc, char *argv[]) {
    if ((argc != 2) && (argc != 4)) {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }

    // Local port must always be specified, since if we use an ephemeral port we have no way
    // to determine what that port is.
    const uint16_t localPort = std::stoi(std::string(argv[1]));

    // Open UDP socket
    UDPSocket udpSocket;
    auto err = udpSocket.open(&OnboardNetworkStack::get_default_instance());
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to open UDP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    err = udpSocket.bind(localPort);
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to bind UDP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    printf(">> Now listening for UDP data on port %" PRIu16 "\n", localPort);

    // Filter by remote address if specified
    if(argc == 4) {
        const uint16_t remotePort = std::stoi(std::string(argv[3]));
        const SocketAddress remoteAddr(argv[2], remotePort);
        err = udpSocket.connect(remoteAddr);
        if(err != NSAPI_ERROR_OK) {
            printf("Failed to connect UDP socket: %s\n", nsapi_strerror(err));
            return CMDLINE_RETCODE_FAIL;
        }

        printf(">> Only accepting packets from remote address %s:%" PRIu16 "\n", remoteAddr.get_ip_address(), remotePort);
    }

    while(true) {
        SocketAddress sourceAddress;
        auto recvResult = udpSocket.recvfrom(&sourceAddress, rxPacketBuffer, sizeof(rxPacketBuffer) - 1);
        if(recvResult < 0) {
            printf("Failed to receive UDP packet: %s\n", nsapi_strerror(recvResult));
        }
        else {
            // Ensure data is null terminated
            rxPacketBuffer[recvResult] = '\0';
            printf("Rx from %s: %s", sourceAddress.get_ip_address(), rxPacketBuffer);
        }
    }
}

int tcp_send(int argc, char *argv[]) {
    if (argc != 4) {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }

    // Append a newline to the payload so it will show up more cleanly in netcat at the other end
    std::string payload(argv[3]);
    payload += "\n";

    // Open TCP socket
    TCPSocket tcpSocket;
    auto err = tcpSocket.open(&OnboardNetworkStack::get_default_instance());
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to open TCP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    // Determine dest IP and port
    uint16_t destPort = std::stoi(std::string(argv[2]));
    SocketAddress dest(argv[1], destPort);
    printf("Sending %zu bytes to %s:%" PRIu16 "\n", payload.size(), dest.get_ip_address(), dest.get_port());

    // Connect to the specified destination
    err = tcpSocket.connect(dest);
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to connect TCP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    err = tcpSocket.send(payload.data(), payload.size());
    if(err < 0) {
        printf("Failed to send: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    return CMDLINE_RETCODE_SUCCESS;
}

int http_request_test(int argc, char *argv[]) {
    if (argc != 1) {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }

    // Open TCP socket
    TCPSocket tcpSocket;
    auto err = tcpSocket.open(&OnboardNetworkStack::get_default_instance());
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to open TCP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    // Resolve the IP address of ifconfig.io
    SocketAddress serverAddr;
    err = OnboardNetworkStack::get_default_instance().gethostbyname("ifconfig.io", &serverAddr);
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to resolve host via DNS: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    // Connect the socket to ifconfig.io
    serverAddr.set_port(80); // HTTP port
    err = tcpSocket.connect(serverAddr);
    if(err != NSAPI_ERROR_OK) {
        printf("Failed to connect TCP socket: %s\n", nsapi_strerror(err));
        return CMDLINE_RETCODE_FAIL;
    }

    // Send a simple http request
    char sbuffer[] = "GET / HTTP/1.1\r\nHost: ifconfig.io\r\n\r\n";
    auto count_or_err = tcpSocket.send(sbuffer, sizeof sbuffer);
    if(count_or_err < 0) {
        printf("Failed to send: %s\n", nsapi_strerror(count_or_err));
        return CMDLINE_RETCODE_FAIL;
    }
    printf("sent %d [%.*s]\n", count_or_err, strstr(sbuffer, "\r\n") - sbuffer, sbuffer);

    // Receive a simple http response and print out the response line
    char rbuffer[64];
    count_or_err = tcpSocket.recv(rbuffer, sizeof rbuffer);
    if(count_or_err < 0) {
        printf("Failed to recv: %s\n", nsapi_strerror(count_or_err));
        return CMDLINE_RETCODE_FAIL;
    }
    printf("recv %d [%.*s]\n", count_or_err, strstr(rbuffer, "\r\n") - rbuffer, rbuffer);

    return CMDLINE_RETCODE_SUCCESS;
}
