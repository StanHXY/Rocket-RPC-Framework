#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include <string>
#include <queue>
#include <memory>
#include <semaphore.h>

#include "rocket/common/config.h"
#include "rocket/common/mutex.h"
#include "rocket/net/timer_event.h"

namespace rocket {


template<typename... Args>
std::string formatString(const char* str, Args&&... args) {

  int size = snprintf(nullptr, 0, str, args...);

  std::string result;
  if (size > 0) {
    result.resize(size);
    snprintf(&result[0], size + 1, str, args...);
  }

  return result;
}


#define DEBUGLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() && rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Debug) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Debug).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \


#define INFOLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Info) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Info).toString() \
    + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

#define ERRORLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Error) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Error).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \


#define APPDEBUGLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Debug) \
  { \
    rocket::Logger::GetGlobalLogger()->pushAppLog(rocket::LogEvent(rocket::LogLevel::Debug).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \


#define APPINFOLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Info) \
  { \
    rocket::Logger::GetGlobalLogger()->pushAppLog(rocket::LogEvent(rocket::LogLevel::Info).toString() \
    + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

#define APPERRORLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Error) \
  { \
    rocket::Logger::GetGlobalLogger()->pushAppLog(rocket::LogEvent(rocket::LogLevel::Error).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \



enum LogLevel {
  Unknown = 0,
  Debug = 1,
  Info = 2,
  Error = 3
};


std::string LogLevelToString(LogLevel level);

LogLevel StringToLogLevel(const std::string& log_level);

#include <queue>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <vector>
#include <memory>

class AsyncLogger {
public:
  typedef std::shared_ptr<AsyncLogger> s_ptr;

  AsyncLogger(const std::string& file_name, const std::string& file_path, int max_size);

  void stop();

  // Flush the logs to disk
  void flush();

  void pushLogBuffer(std::vector<std::string>& vec);

public:
  static void* Loop(void*);

public:
  pthread_t m_thread;

private:
  // m_file_path/m_file_name_yyyymmdd.0

  std::queue<std::vector<std::string>> m_buffer;

  std::string m_file_name;  // Log output file name
  std::string m_file_path;  // Log output path
  int m_max_file_size {0};  // Maximum size of a single log file in bytes

  sem_t m_semaphore;

  pthread_cond_t m_condition;  // Condition variable
  Mutex m_mutex;

  std::string m_date;  // Current file date for log printing
  FILE* m_file_handler {NULL};  // Current opened log file handler

  bool m_reopen_flag {false};

  int m_no {0};  // Log file number

  bool m_stop_flag {false};
};


class Logger {
 public:
  typedef std::shared_ptr<Logger> s_ptr;

  Logger(LogLevel level, int type = 1);

  void pushLog(const std::string& msg);

  void pushAppLog(const std::string& msg);

  void init();

  void log();

  LogLevel getLogLevel() const {
    return m_set_level;
  }

  AsyncLogger::s_ptr getAsyncAppLopger() {
    return m_asnyc_app_logger;
  }

  AsyncLogger::s_ptr getAsyncLopger() {
    return m_asnyc_logger;
  }

  void syncLoop();

  void flush();

 public:
  static Logger* GetGlobalLogger();

  static void InitGlobalLogger(int type = 1);

 private:
  LogLevel m_set_level;
  std::vector<std::string> m_buffer;

  std::vector<std::string> m_app_buffer;

  Mutex m_mutex;

  Mutex m_app_mutex;

  // m_file_path/m_file_name_yyyymmdd.1

  std::string m_file_name;     // Log output file name
  std::string m_file_path;     // Log output path
  int m_max_file_size {0};     // Maximum size of a single log file in bytes


  AsyncLogger::s_ptr m_asnyc_logger;

  AsyncLogger::s_ptr m_asnyc_app_logger;

  TimerEvent::s_ptr m_timer_event;

  int m_type {0};

};


class LogEvent {
 public:

  LogEvent(LogLevel level) : m_level(level) {}

  std::string getFileName() const {
    return m_file_name;  
  }

  LogLevel getLogLevel() const {
    return m_level;
  }

  std::string toString();


private:
  std::string m_file_name;  // File name
  int32_t m_file_line;      // Line number
  int32_t m_pid;            // Process ID
  int32_t m_thread_id;      // Thread ID

  LogLevel m_level;         // Log level


};



}

#endif