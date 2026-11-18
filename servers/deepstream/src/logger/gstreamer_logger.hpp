#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class GstreamerLogger {
public:
    GstreamerLogger();

    void start(const std::string& name);
    void stop();

private:
    std::string pipelineName;
    std::mutex mutex;
    std::condition_variable cv;
    std::thread thread;
    bool running;
    int logFd;
    int pipeRead;

    std::string logFileName() const;
    bool dateOk(const std::string& date) const;
    std::string cutoffDate() const;
    void trimLog() const;
    std::chrono::system_clock::time_point nextMidnight() const;
    int openLog() const;
    bool active();
    bool unlinked() const;
    void adoptLog(int fd) const;
    void redirectLog();
    void waitPipe();
    void pullPipe(std::string& pending);
    void drainLog(std::string& pending);
    void writeLog(const char* data, std::size_t size);
    void rotateLog();
    void rotateDaily();
};
