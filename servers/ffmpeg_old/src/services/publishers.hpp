#pragma once

#include <chrono>
#include <expected>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace services {

enum class FailureKind {
    Rejected,
    Missing,
};

struct Failure {
    FailureKind kind;
    std::string message;
};

struct Publication {
    std::string name;
    std::string rtspUrl;
    std::string command;
};

class Process {
public:
    static std::expected<Process, std::string> start(std::vector<std::string> arguments);

    Process(Process&& other) noexcept;
    Process& operator=(Process&& other) noexcept;
    ~Process();

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    bool running();
    bool exitedWithin(std::chrono::milliseconds timeout);
    std::string failureDetail() const;
    void stop();

private:
    Process(int pid, int stderrFd);

    void reap(int status);
    void closeStderr();

    int pid;
    int stderrFd;
    int exitCode;
    std::string stderrText;
};

class Publishers {
public:
    std::expected<Publication, Failure> publish(
        const std::filesystem::path& input,
        std::string_view name,
        bool loop,
        std::string_view mediamtxHost,
        int mediamtxPort
    );
    std::map<std::string, std::string> list();
    std::map<std::string, std::string> stopAll();
    std::expected<Publication, Failure> stopOne(std::string_view name);

private:
    struct Entry {
        Process process;
        std::filesystem::path input;
        std::string name;
        std::string rtspUrl;
        bool loop;
    };

    std::string resolveName(std::string_view name, const std::filesystem::path& input) const;
    std::vector<std::string> argumentsFor(
        const std::filesystem::path& input,
        bool loop,
        std::string_view rtspUrl
    ) const;
    void cleanup();

    std::mutex mutex;
    std::map<std::string, Entry> entries;
};

}
