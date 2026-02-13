# Mbed NTP CLI Example
This project shows how to use [mbed-client-cli](https://github.com/PelionIoT/mbed-client-cli) to implement a command-line interface, and how to use [mbed-ntp-client](https://github.com/mbed-ce-libraries-examples/mbed-ntp-client) to synchronize the time on your local device with the network.

## How to set up this project:

1. Clone it to your machine:
    ```
    $ git clone https://github.com/mbed-ce-libraries-examples/mbed-ntp-cli-example.git
    $ git submodule update --init mbed-os
    ```
2. You may want to update the mbed-os submodule to the latest version, with `cd mbed-ntp-cli-example/mbed-os && git fetch origin && git reset --hard origin/main`
3. Set up the GNU ARM toolchain (and other programs) on your machine using [the toolchain setup guide](https://github.com/mbed-ce/mbed-os/wiki/Toolchain-Setup-Guide).
4. Set up the CMake project for editing.  We have three ways to do this:
    - On the [command line](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-Command-Line)
    - Using the [CLion IDE](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-CLion)
    - Using the [VS Code IDE](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-VS-Code)
5. Build the `flash-mbed-ntp-cli-example` target to upload the code to a connected device.

## Documentation:

Documentation for the NTP client can be found in [its header file](https://github.com/mbed-ce-libraries-examples/mbed-ntp-client/blob/main/NTPClient.h). Documentation for Mbed CLI can be found in its [README](https://github.com/PelionIoT/mbed-client-cli) and its [header file](https://github.com/PelionIoT/mbed-client-cli/blob/master/mbed-client-cli/ns_cmdline.h).

## How to use the example:

This example can be run on any board that has a built-in Ethernet port or Wi-Fi module. It currently does not support cellular modules or external Ethernet/Wi-Fi modules, though this could be added at some point.'

First, open a serial terminal to your board. Note that to use all the features of Mbed CLI, you will need to use a terminal program that supports VT100 escape sequences, such as TeraTerm on Windows and Minicom on Mac/Linux. Ensure that the program is configured for 115200 baud and LF line endings.

You should see a prompt like:
```
Mbed CE NTP Command Line
/> 
```
(note that you may need to reset the board after connecting your serial terminal for it to display)

Typing `help` will list the available commands:

```
/>help 
Commands:
set             print or set variables                                                                  
unset           unset variables                                                                         
help            This help                                                                               
echo            Echo controlling                                                                        
alias           Handle aliases                                                                          
clear           Clears the display                                                                      
history         View your command Line History                                                          
true                                                                                                    
false                                                                                                   
reboot          Reboots the system using an NVIC system reset                                           
ipconfig        Display network configuration.                                                          
get-time        Print the current NTP time to the console.                                              
ntp-update      Update the local time using NTP.                                                        
unsync-time     Reset the local time back to 1970, clearing the RTC and the NTP sync data.              
eth-connect     Connect to Ethernet, optionally specifying the IP address.
wifi-scan       Scan for Wi-Fi networks.
wifi-connect    Connect to Wi-Fi. IP address will be obtained from DHCP.
```

Note that the commands from `set` through `false` are built-in shell commands provided by mbed-cli, while all the following commands are implemented in this project.

If your board uses Ethernet, use `eth-connect` to initiate the Ethernet connection. You can optionally specify the IP settings to use.

If your board uses wi-fi, use `wifi-scan` to list wifi networks, then `wifi-connect` to connect to one. You must pass the network name, password, and security type. Note that wifi-connect currently does not support using manual IP settings, but this could be added at some point.

Once you are connected to the network, you can use `ipconfig` to display network data:

```
/>ipconfig                                                                                              
Network connection en0:                                                                                 
    Physical Address. . . . : 00:80:e1:32:00:43                                                         
    IP Address. . . . . . . : 192.168.0.133                                                             
    Subnet Mask . . . . . . : 255.255.255.0                                                             
    Default Gateway . . . . : 192.168.0.1                                                               
    DNS Server(s) . . . . . : 192.168.0.1, 192.168.0.1
```

Now, you can use `ntp-update` to perform a time update via NTP. The first time you run it, you can expect a large time offset:

```
/>ntp-update 
Initialized NTP client. The time is now 1970-01-01 00:00:21:000
NTP update successful.
Compensated offset of 1.77e+09 seconds. The time is now 2026-01-04 20:28:14:176
```

On subsequent executions, you should see an offset in the millisecond range at most:
```
/>ntp-update 
NTP update successful.
Compensated offset of 2.20e-02 seconds. The time is now 2026-01-04 20:28:21:064
/>ntp-update 
NTP update successful.
Compensated offset of 0.00e+00 seconds. The time is now 2026-01-04 20:28:23:403
```

Note that `coreSNTP` only has a resolution of 1ms when computing the offset, so if we were already within 0.5ms of the NTP server, it will show zero offset.

If you would like to check the RTC functionality of your board, you can use `reboot` followed by `get-time` to see what time the NTP client loads out of the RTC.

```
/>reboot
Mbed CE NTP Command Line
/>get-time 
Initialized NTP client. The time is now 2026-01-05 17:13:13:000
2026-01-05 17:13:13:000
```

Since this board has RTC functionality, it is able to keep time (to the nearest second) after a reset. If the RTC was not available, this would print a time in 1970.
