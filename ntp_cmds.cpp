
#include "network_connection_cmds.h"
#include "ns_cmdline.h"

#include "ntp_cmds.h"

#include "NTPClient.h"

#include <cinttypes>
#include <OnboardNetworkStack.h>

static bool ntp_initialized = false;

/// Print the current NTP time to the console
static void print_time() {
    // Get time since UNIX epoch in us from NTP client
    const auto usUnixTs = NTPClient::instance().now();

    // Convert to whole seconds and subseconds (since strftime+localtime needs a time_t in seconds)
    time_t wholeSeconds = std::chrono::floor<std::chrono::seconds>(usUnixTs.time_since_epoch()).count();
    const int64_t milliseconds = std::chrono::round<std::chrono::milliseconds>(usUnixTs.time_since_epoch() % 1s).count();

    // Format time as a string
    char timeString[std::size("yyyy-mm-dd hh:mm:ss")];
    std::strftime(timeString, sizeof(timeString), "%F %T", std::localtime(&wholeSeconds));

    // Print time to console
    printf("%s:%03" PRIi64 "\n", timeString, milliseconds);

}

static void init_ntp_if_needed() {
    if (!ntp_initialized) {
        NTPClient::instance().init(&OnboardNetworkStack::get_default_instance());
        ntp_initialized = true;

        printf("Initialized NTP client. The time is now ");
        print_time();
    }
}

int get_time(int argc, char *argv[]) {
    init_ntp_if_needed();
    print_time();
    return CMDLINE_RETCODE_SUCCESS;
}

int ntp_update(int argc, char *argv[]) {
    init_ntp_if_needed();

    std::chrono::microseconds offset;
    auto ret = NTPClient::instance().syncTime(&offset);

    if (ret == SntpSuccess) {
        printf("NTP update successful.\n");
        printf("Compensated offset of %.02e seconds. The time is now ", std::chrono::duration_cast<std::chrono::duration<float>>(offset).count());
        print_time();
        return CMDLINE_RETCODE_SUCCESS;
    }
    else {
        printf("NTP update failed with error %s\n", Sntp_StatusToStr(ret));
        return CMDLINE_RETCODE_FAIL;
    }
}

int unsync_time(int argc, char *argv[]) {
    init_ntp_if_needed();
    RealTimeClock::write(RealTimeClock::time_point(0s));
    NTPClient::instance().set_offset(0ms);
    return CMDLINE_RETCODE_SUCCESS;
}
