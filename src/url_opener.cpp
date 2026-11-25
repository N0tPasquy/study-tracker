#include "url_opener.hpp"

#include <algorithm>
#include <cctype>

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#elif defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif

namespace tracker {

bool is_valid_http_url(std::string_view url) {
    constexpr std::string_view prefix_http = "http://";
    constexpr std::string_view prefix_https = "https://";

    std::string_view rest;
    if (url.starts_with(prefix_https)) {
        rest = url.substr(prefix_https.size());
    } else if (url.starts_with(prefix_http)) {
        rest = url.substr(prefix_http.size());
    } else {
        return false;
    }

    if (rest.empty()) {
        return false;
    }

    for (unsigned char c : url) {
        // Disallow whitespace and ASCII control characters
        if (c <= 32 || c == 127) {
            return false;
        }
        // Disallow dangerous shell characters
        if (c == '"' || c == '\'' || c == '`' || c == ';' || c == '\n' || c == '\r' || c == '\0') {
            return false;
        }
    }

    return true;
}

bool open_url_in_browser(const std::string& url) {
    if (!is_valid_http_url(url)) {
        return false;
    }

#if defined(__APPLE__) || defined(__linux__)
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        // Child: fork again (double-fork pattern to prevent zombie processes)
        pid_t grand_pid = fork();
        if (grand_pid < 0) {
            _exit(1);
        }
        if (grand_pid > 0) {
            // First child exits immediately, reaped by parent
            _exit(0);
        }

        // Grandchild:
        // Detach session
        setsid();

        // Redirect standard file descriptors to /dev/null
        int dev_null = open("/dev/null", O_RDWR);
        if (dev_null >= 0) {
            dup2(dev_null, STDIN_FILENO);
            dup2(dev_null, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);
            if (dev_null > STDERR_FILENO) {
                close(dev_null);
            }
        }

#if defined(__APPLE__)
        const char* cmd = "open";
#else
        const char* cmd = "xdg-open";
#endif
        // POSIX execvp requires a non-const `char* const argv[]` signature for historical reasons;
        // it does not mutate the argument strings.
        char* const args[] = {
            const_cast<char*>(cmd),
            const_cast<char*>(url.c_str()),
            nullptr
        };

        execvp(cmd, args);
        _exit(1);
    }

    // Parent: wait for first child which exits immediately
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && (WEXITSTATUS(status) == 0);

#elif defined(_WIN32)
    HINSTANCE res = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(res) > 32;

#else
    return false;
#endif
}

} // namespace tracker
