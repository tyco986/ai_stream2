#include "logger/gstreamer_logger.hpp"

#include "constants/gstreamer_logger_const.hpp"

#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

GstreamerLogger::GstreamerLogger()
    : pipelineName(),
      running(false),
      logFd(-1),
      pipeRead(-1) {
}

void GstreamerLogger::start(const std::string& name) {
    pipelineName = name;
    redirectLog();
    {
        std::lock_guard<std::mutex> lock(mutex);
        running = true;
    }
    thread = std::thread(&GstreamerLogger::rotateDaily, this);
}

void GstreamerLogger::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        running = false;
    }
    cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}

std::string GstreamerLogger::logFileName() const {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_r(&now, &local);
    char date[gstreamer_log::DATE_SIZE];
    std::strftime(date, sizeof(date), gstreamer_log::DATE_FORMAT, &local);
    return std::string(gstreamer_log::FILE_PREFIX) + date + gstreamer_log::FILE_SUFFIX;
}

bool GstreamerLogger::dateOk(const std::string& date) const {
    bool ok = date.size() == gstreamer_log::DATE_LENGTH;
    for (std::size_t index = 0; ok && index < date.size(); ++index) {
        const bool separator = index == 4 || index == 7;
        const unsigned char character = static_cast<unsigned char>(date[index]);
        ok = separator ? date[index] == '-' : std::isdigit(character) != 0;
    }
    return ok;
}

std::string GstreamerLogger::cutoffDate() const {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_r(&now, &local);
    local.tm_mday -= static_cast<int>(gstreamer_log::MAX_DAYS);
    std::mktime(&local);
    char date[gstreamer_log::DATE_SIZE];
    std::strftime(date, sizeof(date), gstreamer_log::DATE_FORMAT, &local);
    return std::string(date);
}

void GstreamerLogger::trimLog() const {
    const std::filesystem::path directory =
        std::filesystem::path(gstreamer_log::ROOT) / pipelineName / gstreamer_log::DIRECTORY;
    const bool listed = std::filesystem::is_directory(directory);
    if (listed) {
        const std::string cutoff = cutoffDate();
        const std::string prefix = gstreamer_log::FILE_PREFIX;
        const std::string suffix = gstreamer_log::FILE_SUFFIX;
        const std::size_t length = prefix.size() + gstreamer_log::DATE_LENGTH + suffix.size();
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory)) {
            const std::string filename = entry.path().filename().string();
            const bool shaped = entry.is_regular_file() && filename.size() == length &&
                filename.compare(0, prefix.size(), prefix) == 0 &&
                filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
            const std::string stamp = shaped ? filename.substr(prefix.size(), gstreamer_log::DATE_LENGTH) : "";
            const bool expired = dateOk(stamp) && stamp <= cutoff;
            if (expired) {
                std::filesystem::remove(entry.path());
            }
        }
    }
}

std::chrono::system_clock::time_point GstreamerLogger::nextMidnight() const {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_r(&now, &local);
    local.tm_hour = 0;
    local.tm_min = 0;
    local.tm_sec = 0;
    local.tm_mday += 1;
    return std::chrono::system_clock::from_time_t(std::mktime(&local));
}

int GstreamerLogger::openLog() const {
    const std::filesystem::path directory =
        std::filesystem::path(gstreamer_log::ROOT) / pipelineName / gstreamer_log::DIRECTORY;
    std::filesystem::create_directories(directory);
    const std::filesystem::path file = directory / logFileName();
    const std::filesystem::path candidates[] = {
        directory / gstreamer_log::FILE_NAME,
        directory.parent_path() / gstreamer_log::LEGACY_FILE,
    };
    for (const std::filesystem::path& candidate : candidates) {
        const bool migrate = !std::filesystem::exists(file) &&
            std::filesystem::symlink_status(candidate).type() == std::filesystem::file_type::regular;
        if (migrate) {
            std::filesystem::rename(candidate, file);
        }
    }
    trimLog();
    return ::open(file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
}

bool GstreamerLogger::active() {
    std::lock_guard<std::mutex> lock(mutex);
    return running;
}

bool GstreamerLogger::unlinked() const {
    const int fd = pipeRead >= 0 ? logFd : STDOUT_FILENO;
    struct stat state {};
    const int result = fd < 0 ? -1 : ::fstat(fd, &state);
    const bool gone = result == 0 && state.st_nlink == 0;
    return gone;
}

void GstreamerLogger::adoptLog(int fd) const {
    const bool opened = fd >= 0;
    if (opened) {
        ::dup2(fd, STDOUT_FILENO);
        ::dup2(fd, STDERR_FILENO);
        ::close(fd);
    }
}

void GstreamerLogger::redirectLog() {
    const int opened = openLog();
    int ends[2] = {-1, -1};
    const int piped = opened < 0 ? -1 : ::pipe(ends);
    const bool ready = opened >= 0 && piped == 0;
    if (ready) {
        ::fcntl(ends[0], F_SETFL, O_NONBLOCK);
        ::dup2(ends[1], STDOUT_FILENO);
        ::dup2(ends[1], STDERR_FILENO);
        ::close(ends[1]);
        pipeRead = ends[0];
        logFd = opened;
        std::setvbuf(stdout, nullptr, _IOLBF, 0);
        std::setvbuf(stderr, nullptr, _IOLBF, 0);
    }
    if (opened >= 0 && !ready) {
        adoptLog(opened);
        std::setvbuf(stdout, nullptr, _IOLBF, 0);
        std::setvbuf(stderr, nullptr, _IOLBF, 0);
    }
}

void GstreamerLogger::waitPipe() {
    const bool piped = pipeRead >= 0;
    if (piped) {
        pollfd slot{};
        slot.fd = pipeRead;
        slot.events = POLLIN;
        ::poll(&slot, 1, gstreamer_log::REOPEN_SECONDS * 1000);
    }
    if (!piped) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait_for(lock, std::chrono::seconds(gstreamer_log::REOPEN_SECONDS));
    }
}

void GstreamerLogger::writeLog(const char* data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size && logFd >= 0) {
        const ssize_t wrote = ::write(logFd, data + offset, size - offset);
        if (wrote > 0) {
            offset += static_cast<std::size_t>(wrote);
        }
        if (wrote < 0 && errno != EINTR) {
            offset = size;
        }
    }
}

void GstreamerLogger::drainLog(std::string& pending) {
    const std::size_t prefixSize = sizeof(gstreamer_log::DROP_PREFIX) - 1;
    std::size_t newline = pending.find('\n');
    while (newline != std::string::npos) {
        const bool drop = pending.compare(0, prefixSize, gstreamer_log::DROP_PREFIX) == 0;
        if (!drop) {
            writeLog(pending.data(), newline + 1);
        }
        pending.erase(0, newline + 1);
        newline = pending.find('\n');
    }
    const bool overflow = pending.size() >= gstreamer_log::MAX_LINE;
    const bool drop = overflow && pending.compare(0, prefixSize, gstreamer_log::DROP_PREFIX) == 0;
    if (overflow && !drop) {
        writeLog(pending.data(), pending.size());
    }
    if (overflow) {
        pending.clear();
    }
}

void GstreamerLogger::pullPipe(std::string& pending) {
    char buffer[gstreamer_log::READ_SIZE];
    ssize_t count = pipeRead >= 0 ? 1 : 0;
    while (count > 0) {
        count = ::read(pipeRead, buffer, sizeof(buffer));
        if (count > 0) {
            pending.append(buffer, static_cast<std::size_t>(count));
            drainLog(pending);
        }
        const bool again = count < 0 && errno == EINTR;
        count = again ? 1 : count;
    }
}

void GstreamerLogger::rotateLog() {
    const bool piped = pipeRead >= 0;
    if (piped) {
        ::flockfile(stdout);
        ::flockfile(stderr);
        std::fflush(stdout);
        std::fflush(stderr);
        ::funlockfile(stderr);
        ::funlockfile(stdout);
    }
    const int opened = openLog();
    if (opened >= 0 && piped) {
        const int previous = logFd;
        logFd = opened;
        if (previous >= 0) {
            ::close(previous);
        }
    }
    if (opened >= 0 && !piped) {
        ::flockfile(stdout);
        ::flockfile(stderr);
        std::fflush(stdout);
        std::fflush(stderr);
        adoptLog(opened);
        ::funlockfile(stderr);
        ::funlockfile(stdout);
    }
}

// Libraries write straight to the pipe. This thread drops encoder latency lines and reopens the file at local midnight, or when that file was unlinked.
void GstreamerLogger::rotateDaily() {
    std::string pending;
    while (active()) {
        const auto deadline = nextMidnight();
        waitPipe();
        const bool due = std::chrono::system_clock::now() >= deadline;
        const bool gone = unlinked();
        const bool turn = active() && (due || gone);
        if (turn) {
            rotateLog();
        }
        pullPipe(pending);
    }
    const std::size_t prefixSize = sizeof(gstreamer_log::DROP_PREFIX) - 1;
    const bool rest = !pending.empty() && pending.compare(0, prefixSize, gstreamer_log::DROP_PREFIX) != 0;
    if (rest) {
        writeLog(pending.data(), pending.size());
    }
}
