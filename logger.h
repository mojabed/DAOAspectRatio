#pragma once

#include <string>
#include <fstream>
#include <source_location>
#include <mutex>

class Logger {
public:
    Logger();
    ~Logger();
    static Logger& getInstance();

	void timestamp(std::ostream& os) const;

    void logError(const std::string& message,
                  const std::source_location& location = std::source_location::current()) const;
    void log(const std::string& message,
                  const std::source_location& location = std::source_location::current()) const;

private:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getLastErrorAsString() const;

    const std::string logFileName = "d3d9_hook.log";
	static const std::ios_base::openmode append = std::ios_base::app;
	static const std::ios_base::openmode clear = std::ios_base::trunc;
    mutable std::ofstream logFile;
    static std::mutex logMutex;
};