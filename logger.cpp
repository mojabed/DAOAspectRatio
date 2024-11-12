#include "logger.h"
#include <windows.h>
#include <chrono>
#include <ctime>

std::mutex Logger::logMutex;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

std::string Logger::getLastErrorAsString() const {
	// Retrieve error code from last run function
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0) {
		return "No error message can be retrieved";
	}

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	LocalFree(messageBuffer);

	return message;
}

// Provides an optional timestamp to logging
void Logger::timestamp(std::ostream& os) const {
	std::lock_guard<std::mutex> lock(logMutex);

	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm buf;
	localtime_s(&buf, &in_time_t);

	os << std::put_time(&buf, "%Y-%m-%d %H:%M:%S ");
}

void Logger::logError(const std::string& message, const std::source_location& location) const {

	std::string lastErrorAsString = getLastErrorAsString();

	timestamp(logFile);
    logFile << location.file_name() << '('
		    << location.line() << ") '"
		    << location.function_name() << "': "
		    << message << ":: " << lastErrorAsString << std::endl;
}

void Logger::log(const std::string& message, const std::source_location& location) const {
	timestamp(logFile);
	logFile << location.file_name() << '('
		    << location.line() << ") '"
		    << location.function_name() << "': "
		    << message << std::endl;
}

Logger::Logger() {
	// Clear log file on new run
    logFile.open(logFileName, clear);
    logFile.close();
	logFile.open(logFileName, append);
}

Logger::~Logger() {
	if (logFile.is_open()) {
		logFile.close();
	}
}
