#include "network_connection_cmds.h"

#include <EthernetInterface.h>
#include <WiFiInterface.h>
#include <mbed_trace.h>

#include "mbed-client-cli/ns_cmdline.h"

#define TRACE_GROUP "net_cmds"

// Specific network interface
#if MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE == ETHERNET
NetworkInterface & netInterface = *EthernetInterface::get_default_instance();
#elif MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE == WIFI
WiFiInterface & wifiInterface = *WiFiInterface::get_default_instance();
NetworkInterface & netInterface = wifiInterface;
#endif

// Network status callback, to report network status to console
static nsapi_connection_status_t connStatus = NSAPI_STATUS_DISCONNECTED;
void networkStatusCallback(const nsapi_event_t event, intptr_t parameter)
{
    if (event == NSAPI_EVENT_CONNECTION_STATUS_CHANGE)
    {
        connStatus = static_cast<nsapi_connection_status_t>(parameter);
        char const *message;
        switch (connStatus)
        {
            case NSAPI_STATUS_LOCAL_UP:
                message = "Local IP address set";
                break;
            case NSAPI_STATUS_GLOBAL_UP:
                message = "Global IP address set";
                break;
            case NSAPI_STATUS_DISCONNECTED:
                message = "No connection to network";
                break;
            case NSAPI_STATUS_CONNECTING:
                message = "Connecting to network";
                break;
            default:
                message = "Unknown status";
        }

        tr_info("Connection status change: %s", message);
    }
}

#if MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE == WIFI
int wifi_scan(int argc, char *argv[])
{
    constexpr size_t maxNetworks = 15; // each network uses 48 bytes RAM
    std::vector<WiFiAccessPoint> scannedAPs(maxNetworks);

    printf("Scanning for Wi-Fi networks...\n");
    auto ret = wifiInterface.scan(scannedAPs.data(), maxNetworks);
    if(ret < 0) {
        tr_error("Error performing wifi scan: %s", nsapi_strerror(ret));
        return CMDLINE_RETCODE_FAIL;
    }

    if(ret == 0) {
        printf("No networks detected.\n");
    }
    else {
        printf("Detected %d access points: \n", ret);

        // Shrink the vector to fit the number of networks actually seen, then sort by RSSI from high
        // to low.
        scannedAPs.resize(ret);
        auto comparator = [](WiFiAccessPoint const & lhs, WiFiAccessPoint const & rhs) { return lhs.get_rssi() > rhs.get_rssi(); };
        std::sort(scannedAPs.begin(), scannedAPs.end(), comparator);

        for (WiFiAccessPoint& ap : scannedAPs) {
            printf("- SSID: \"%s\", security: %s, RSSI: %" PRIi8 " dBm, Ch: %" PRIu8 "\n", ap.get_ssid(),
               nsapi_security_to_string(ap.get_security()), ap.get_rssi(), ap.get_channel());
        }
    }

    return CMDLINE_RETCODE_SUCCESS;
}

int wifi_connect(int argc, char *argv[]) {
    // Validate arguments
    if (argc < 3) {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }
    const char* ssid = argv[1];
    const auto security = nsapi_string_to_security(argv[2]);
    char* password = nullptr;
    if (security == NSAPI_SECURITY_UNKNOWN) {
        // Invalid security value
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }
    else if (security == NSAPI_SECURITY_NONE) {
        // No password parameter
        if (argc != 3) {
            return CMDLINE_RETCODE_INVALID_PARAMETERS;
        }
    }
    else {
        // Password parameter needed
        if (argc != 4) {
            return CMDLINE_RETCODE_INVALID_PARAMETERS;
        }
        password = argv[3];
    }

    if (const auto ret = wifiInterface.set_credentials(ssid, password, security); ret != NSAPI_ERROR_OK) {
        tr_error("Error setting Wi-Fi credentials: %d", ret);
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }

    wifiInterface.attach(networkStatusCallback);

    printf("Connecting to the Wi-Fi network...\n");
    if (const auto ret = netInterface.connect(); ret != NSAPI_ERROR_OK) {
        tr_error("Error connecting: %d", ret);
        return CMDLINE_RETCODE_FAIL;
    }

    // Print our settings
    printf("Connected to \"%s\".\n", ssid);
    printf("Use 'ipconfig' command to see network information.\n");

    return CMDLINE_RETCODE_SUCCESS;
}
#endif

int eth_connect(int argc, char *argv[]) {
    // Validate arguments
    if (argc != 1 && argc != 4) {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }

    if (argc > 1) {
        SocketAddress ip(argv[1]);
        if (ip.is_empty()) {
            printf("Failed to parse IP address from args.\n");
            return CMDLINE_RETCODE_INVALID_PARAMETERS;
        }

        SocketAddress netmask(argv[2]);
        if (netmask.is_empty()) {
            printf("Failed to parse netmask from args.\n");
            return CMDLINE_RETCODE_INVALID_PARAMETERS;
        }

        SocketAddress gateway(argv[3]);
        if (gateway.is_empty()) {
            printf("Failed to parse gateway from args.\n");
            return CMDLINE_RETCODE_INVALID_PARAMETERS;
        }

        netInterface.set_network(ip, netmask, gateway);
    }

    netInterface.attach(networkStatusCallback);
    printf("Connecting to the Ethernet network...\n");
    if (const auto ret = netInterface.connect(); ret != NSAPI_ERROR_OK) {
        tr_error("Error connecting: %s", nsapi_strerror(ret));
        return CMDLINE_RETCODE_FAIL;
    }

    // Print our settings
    printf("Connected.\n");
    printf("Use 'ipconfig' command to see network information.\n");

    return CMDLINE_RETCODE_SUCCESS;
}

int ipconfig(int argc, char *argv[]) {
    if (connStatus == NSAPI_STATUS_CONNECTING || connStatus == NSAPI_STATUS_DISCONNECTED) {
        printf("Not connected to a network.\n");
        return CMDLINE_RETCODE_FAIL;
    }

    // Get the interface name
    char ifName[6];
    if (netInterface.get_interface_name(ifName) == nullptr) {
        printf("Failed to get interface name\n");
        return CMDLINE_RETCODE_FAIL;
    }

    printf("Network connection %s:\n", ifName);
    if (netInterface.get_mac_address() != nullptr) {
        printf("    Physical Address. . . . : %s\n", netInterface.get_mac_address());
    }

    SocketAddress ip;
    if (netInterface.get_ip_address(&ip) == NSAPI_ERROR_OK) {
        printf("    IP Address. . . . . . . : %s\n", ip.get_ip_address());
    }
    SocketAddress netmask;
    if (ip.get_ip_version() == NSAPI_IPv4 && netInterface.get_netmask(&netmask) == NSAPI_ERROR_OK) {
        printf("    Subnet Mask . . . . . . : %s\n", netmask.get_ip_address());
    }
    SocketAddress gateway;
    if (ip.get_ip_version() == NSAPI_IPv4 && netInterface.get_gateway(&gateway) == NSAPI_ERROR_OK) {
        printf("    Default Gateway . . . . : %s\n", gateway.get_ip_address());
    }
    SocketAddress linkLocalIPv6;
    if (netInterface.get_ipv6_link_local_address(&linkLocalIPv6) == NSAPI_ERROR_OK) {
        printf("    Link-local IPv6 Address : %s\n", linkLocalIPv6.get_ip_address());
    }

    // Print DNS servers, if available
    printf("    DNS Server(s) . . . . . : ");
    SocketAddress dnsServer;
    for (size_t dnsServerIndex = 0; netInterface.get_dns_server(dnsServerIndex, &dnsServer, ifName) == NSAPI_ERROR_OK; dnsServerIndex++) {
        if (dnsServerIndex != 0) {
            printf(", ");
        }
        printf("%s", dnsServer.get_ip_address());
    }
    printf("\n");

    // print wi-fi information
    if (netInterface.wifiInterface() != nullptr) {
        printf("    Wi-Fi RSSI. . . . . . . : %" PRIi8 "\n", netInterface.wifiInterface()->get_rssi());
    }

    return CMDLINE_RETCODE_SUCCESS;
}