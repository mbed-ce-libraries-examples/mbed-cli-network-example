/*
 * Copyright (c) 2018-2019, Pelion and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cstdio>
#include <cstring>

#include "mbed.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed-client-cli/ns_cmdline.h"

#include "network_connection_cmds.h"
#include "ntp_cmds.h"
#include "network_demo_cmds.h"

#define TRACE_GROUP   "main"

static Mutex SerialOutMutex;
void serial_out_mutex_wait()
{
    SerialOutMutex.lock();
}
void serial_out_mutex_release()
{
    SerialOutMutex.unlock();
}

// Command to reboot the system
int reboot(int argc, char *argv[]) {
    NVIC_SystemReset();
}

int main(void)
{
    // Initialize trace library
    mbed_trace_init();

    // Initialize Mbed CLI
    cmd_init(nullptr);

    // Set up Mbed Trace and Mbed CLI to use a mutex for the console (so they don't try to print stuff
    // at the same time)
    mbed_trace_mutex_wait_function_set(serial_out_mutex_wait);
    mbed_trace_mutex_release_function_set(serial_out_mutex_release);
    cmd_mutex_wait_func(serial_out_mutex_wait);
    cmd_mutex_release_func(serial_out_mutex_release);

    // Add commands
    cmd_add("reboot", reboot, "Reboots the system using an NVIC system reset", nullptr);
    cmd_add("ipconfig", ipconfig, "Display network configuration.", nullptr);
    cmd_add("get-time", get_time, "Print the current NTP time to the console.", nullptr);
    cmd_add("ntp-update", ntp_update, "Update the local time using NTP.", nullptr);
    cmd_add("unsync-time", unsync_time, "Reset the local time back to 1970, clearing the RTC and the NTP sync data.", nullptr);
#if MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE == WIFI
    cmd_add("wifi-scan", wifi_scan, "Scan for Wi-Fi networks.", nullptr);
    cmd_add("wifi-connect", wifi_connect, "Connect to Wi-Fi. IP address will be obtained from DHCP.",
        "Usage: wifi-connect <ssid> <security> [password]\n"
        "This will connect to the given wi-fi network with the given SSID, security type, and password.\n"
        "Available networks can be seen by running the `wifi-scan` command.");
#elif MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE == ETHERNET
    cmd_add("eth-connect", eth_connect, "Connect to Ethernet, optionally specifying the IP address.",
        "Usage: eth-connect [<ip> <netmask> <gateway>]\n"
        "Activates the Ethernet port and attempts to connect to the local network.\n"
        "If you run the command with no arguments, DHCP is used to obtain an IP address.\n"
        "Or, you may manually pass an IP, netmask, and gateway.");
#endif
    cmd_add("udp-send", udp_send, "Send a string via UDP packet",
        "Usage: udp-send <dest IP> <dest port> \"<payload to send>\"\n"
        "Sends the given payload string (plus a newline) to a remote host via UDP packet.");
    cmd_add("udp-listen", udp_listen, "Listen forever for UDP packets",
        "Usage: udp-listen <local port> [<remote IP> <remote port>]\n"
        "Enters a loop listening forever for UDP packets on the given port. Optionally, you can pass a specific\n"
        "remote IP and port to only accept packets from this address.");

    // Run the CLI

    cmd_init_screen();
    while (true) {
        int c = getchar();
        if (c != EOF) {
            cmd_char_input(c);
        }
    }
}