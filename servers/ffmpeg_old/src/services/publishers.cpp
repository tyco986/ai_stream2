#include "services/publishers.hpp"

#include <cctype>
#include <chrono>
#include <cstring>
#include <thread>
#include <utility>

#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "constants/constants.hpp"

extern char** environ;

namespace services {
namespace {

std::string trim(std::string_view text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

std::string quoteArgument(std::string_view argument) {
    std::string quoted = "'";
    for (char character : argument) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted += character;
        }
    }
    quoted += "'";
    return quoted;
}

std::string commandLine(const std::vector<std::string>& arguments) {
    std::string line;
    for (const std::string& argument : arguments) {
        if (!line.empty()) {
            line += ' ';
        }
        line += quoteArgument(argument);
    }
    return line;
}

std::string readAll(int fd) {
    std::string text;
    char buffer[4096];
    ssize_t count = 0;
    while (fd >= 0 && (count = ::read(fd, buffer, sizeof buffer)) > 0) {
        text.append(buffer, static_cast<std::size_t>(count));
    }
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

void appendBase(std::vector<std::string>& arguments) {
    for (std::string_view argument : constants::FFMPEG_BASE) {
        arguments.emplace_back(argument);
    }
}

}  // namespace

Process::Process(int pid, int stderrFd)
    : pid(pid), stderrFd(stderrFd), exitCode(-1), stderrText() {}

Process::Process(Process&& other) noexcept
    : pid(other.pid),
      stderrFd(other.stderrFd),
      exitCode(other.exitCode),
      stderrText(std::move(other.stderrText)) {
    other.pid = -1;
    other.stderrFd = -1;
}

Process& Process::operator=(Process&& other) noexcept {
    if (this != &other) {
        stop();
        closeStderr();
        pid = other.pid;
        stderrFd = other.stderrFd;
        exitCode = other.exitCode;
        stderrText = std::move(other.stderrText);
        other.pid = -1;
        other.stderrFd = -1;
    }
    return *this;
}

Process::~Process() {
    stop();
    closeStderr();
}

std::expected<Process, std::string> Process::start(std::vector<std::string> arguments) {
    std::expected<Process, std::string> result = std::unexpected("failed to start ffmpeg");
    int pipeFd[2] = {-1, -1};
    posix_spawn_file_actions_t actions;
    bool actionsReady = posix_spawn_file_actions_init(&actions) == 0;
    bool pipeReady = actionsReady && ::pipe2(pipeFd, O_CLOEXEC) == 0;
    if (pipeReady) {
        posix_spawn_file_actions_adddup2(&actions, pipeFd[1], STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, pipeFd[0]);
        posix_spawn_file_actions_addclose(&actions, pipeFd[1]);
        std::vector<char*> argv;
        argv.reserve(arguments.size() + 1);
        for (std::string& argument : arguments) {
            argv.push_back(argument.data());
        }
        argv.push_back(nullptr);
        pid_t child = -1;
        int spawned = ::posix_spawnp(&child, argv[0], &actions, nullptr, argv.data(), environ);
        ::close(pipeFd[1]);
        if (spawned == 0) {
            result = Process(child, pipeFd[0]);
        } else {
            ::close(pipeFd[0]);
            result = std::unexpected(std::string("failed to start ffmpeg: ") + std::strerror(spawned));
        }
    }
    if (actionsReady) {
        posix_spawn_file_actions_destroy(&actions);
    }
    return result;
}

void Process::reap(int status) {
    exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : status;
    pid = -1;
    stderrText = readAll(stderrFd);
    closeStderr();
}

void Process::closeStderr() {
    if (stderrFd >= 0) {
        ::close(stderrFd);
        stderrFd = -1;
    }
}

bool Process::running() {
    bool alive = false;
    if (pid > 0) {
        int status = 0;
        pid_t waited = ::waitpid(pid, &status, WNOHANG);
        while (waited < 0 && errno == EINTR) {
            waited = ::waitpid(pid, &status, WNOHANG);
        }
        if (waited == 0) {
            alive = true;
        } else if (waited == pid) {
            reap(status);
        } else {
            pid = -1;
        }
    }
    return alive;
}

bool Process::exitedWithin(std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    bool exited = !running();
    while (!exited && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        exited = !running();
    }
    return exited;
}

std::string Process::failureDetail() const {
    std::string detail = stderrText;
    if (detail.empty()) {
        detail = "ffmpeg exited with code " + std::to_string(exitCode);
    }
    return detail;
}

void Process::stop() {
    bool alive = running();
    if (alive) {
        ::kill(pid, SIGTERM);
        bool exited = exitedWithin(constants::PUBLISHER_STOP_TIMEOUT);
        if (!exited) {
            ::kill(pid, SIGKILL);
            int status = 0;
            pid_t waited = ::waitpid(pid, &status, 0);
            while (waited < 0 && errno == EINTR) {
                waited = ::waitpid(pid, &status, 0);
            }
            if (waited == pid) {
                reap(status);
            } else {
                pid = -1;
                closeStderr();
            }
        }
    }
}

std::string Publishers::resolveName(std::string_view name, const std::filesystem::path& input) const {
    std::string streamName = trim(name);
    if (streamName.empty()) {
        streamName = input.stem().string();
    }
    bool invalid = streamName.empty() || streamName.find('/') != std::string::npos
        || streamName.find('\\') != std::string::npos;
    if (invalid) {
        streamName.clear();
    }
    return streamName;
}

std::vector<std::string> Publishers::argumentsFor(
    const std::filesystem::path& input,
    bool loop,
    std::string_view rtspUrl
) const {
    std::vector<std::string> arguments;
    appendBase(arguments);
    arguments.emplace_back("-re");
    arguments.emplace_back("-fflags");
    arguments.emplace_back("+genpts");
    if (loop) {
        arguments.emplace_back("-stream_loop");
        arguments.emplace_back("-1");
    }
    arguments.emplace_back("-i");
    arguments.emplace_back(input.string());
    arguments.emplace_back("-c:v");
    arguments.emplace_back("copy");
    arguments.emplace_back("-an");
    arguments.emplace_back("-f");
    arguments.emplace_back("rtsp");
    arguments.emplace_back("-rtsp_transport");
    arguments.emplace_back("tcp");
    arguments.emplace_back(std::string(rtspUrl));
    return arguments;
}

void Publishers::cleanup() {
    std::erase_if(entries, [](auto& item) { return !item.second.process.running(); });
}

std::expected<Publication, Failure> Publishers::publish(
    const std::filesystem::path& input,
    std::string_view name,
    bool loop,
    std::string_view mediamtxHost,
    int mediamtxPort
) {
    std::string streamName = resolveName(name, input);
    std::string host(mediamtxHost.empty() ? constants::MEDIAMTX_HOST : mediamtxHost);
    int port = mediamtxPort == 0 ? constants::MEDIAMTX_PORT : mediamtxPort;
    std::string rtspUrl = "rtsp://" + host + ":" + std::to_string(port) + "/" + streamName;
    std::expected<Publication, Failure> result =
        std::unexpected(Failure{FailureKind::Rejected, "name must be a non-empty path segment"});
    if (!streamName.empty()) {
        std::lock_guard lock(mutex);
        cleanup();
        if (entries.contains(streamName)) {
            result = std::unexpected(
                Failure{FailureKind::Rejected, "publisher already exists: " + streamName}
            );
        } else {
            std::vector<std::string> arguments = argumentsFor(input, loop, rtspUrl);
            std::string command = commandLine(arguments);
            std::expected<Process, std::string> started = Process::start(std::move(arguments));
            if (started) {
                bool failed = started->exitedWithin(constants::PUBLISHER_START_TIMEOUT);
                std::string detail = started->failureDetail();
                if (failed) {
                    result = std::unexpected(Failure{FailureKind::Rejected, detail});
                } else {
                    Publication publication{streamName, rtspUrl, command};
                    entries.emplace(
                        streamName,
                        Entry{std::move(*started), input, streamName, rtspUrl, loop}
                    );
                    result = publication;
                }
            } else {
                result = std::unexpected(Failure{FailureKind::Rejected, started.error()});
            }
        }
    }
    return result;
}

std::map<std::string, std::string> Publishers::list() {
    std::lock_guard lock(mutex);
    cleanup();
    std::map<std::string, std::string> active;
    for (const auto& [name, entry] : entries) {
        active.emplace(name, entry.rtspUrl);
    }
    return active;
}

std::map<std::string, std::string> Publishers::stopAll() {
    std::map<std::string, std::string> stopped;
    std::vector<Process> processes;
    {
        std::lock_guard lock(mutex);
        cleanup();
        for (auto& [name, entry] : entries) {
            stopped.emplace(name, entry.rtspUrl);
            processes.push_back(std::move(entry.process));
        }
        entries.clear();
    }
    for (Process& process : processes) {
        process.stop();
    }
    return stopped;
}

std::expected<Publication, Failure> Publishers::stopOne(std::string_view name) {
    std::expected<Publication, Failure> result = std::unexpected(
        Failure{FailureKind::Missing, "publisher not found: " + std::string(name)}
    );
    std::vector<Process> processes;
    {
        std::lock_guard lock(mutex);
        cleanup();
        auto found = entries.find(std::string(name));
        if (found != entries.end()) {
            result = Publication{found->second.name, found->second.rtspUrl, ""};
            processes.push_back(std::move(found->second.process));
            entries.erase(found);
        }
    }
    for (Process& process : processes) {
        process.stop();
    }
    return result;
}

}  // namespace services
