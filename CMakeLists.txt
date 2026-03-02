cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19...3.22)

set(MBED_APP_JSON_PATH mbed_app.json5)

include(mbed-os/tools/cmake/mbed_toolchain_setup.cmake)
project(mbed-cli-network-example
    LANGUAGES C CXX ASM) 
include(mbed_project_setup)

add_subdirectory(mbed-os)

# Pull in Mbed Client CLI submodule
mbed_setup_submodule(mbed-client-cli CHECK_FILE source/ns_cmdline.c)
add_subdirectory(mbed-client-cli)

# Pull in NTP Client submodule
mbed_setup_submodule(mbed-ntp-client CHECK_FILE NTPClient.cpp)
add_subdirectory(mbed-ntp-client)

add_executable(${PROJECT_NAME}
        main.cpp
        network_connection_cmds.cpp
        ntp_cmds.cpp
        network_demo_cmds.cpp
)
target_link_libraries(${PROJECT_NAME}
        mbed-os
        mbed-client-cli
        mbed-ntp-client
        mbed-netsocket
)

# Error on missing return values (I make this mistake a lot...)
target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Werror=return-type)

mbed_set_post_build(${PROJECT_NAME})