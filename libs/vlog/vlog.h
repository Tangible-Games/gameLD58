#pragma once

#include <array>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <string_view>

#ifndef VLOG_ENABLED
#define VLOG_ENABLED 1
#endif

namespace vlog {

class LoggerSink {
 public:
  LoggerSink() = default;
  virtual ~LoggerSink() = default;

  virtual LoggerSink& out(std::string_view string, bool flush = false) = 0;
  void endl() { out("\n", true); }
};

class ConsoleSink : public LoggerSink {
 public:
  static std::unique_ptr<LoggerSink> create() {
    auto sink = std::unique_ptr<LoggerSink>(new ConsoleSink);
#if VLOG_ENABLED
    sink->out(std::format("--- [{}] vlog::ConsoleSink started ---\n",
                          std::chrono::system_clock::now()),
              true);
#endif
    return sink;
  }
  virtual ~ConsoleSink() override = default;

  LoggerSink& out(std::string_view string,
                  [[maybe_unused]] bool flush = false) override {
    std::cerr << string;
    return *this;
  }

 private:
  ConsoleSink() = default;
};

class FileSink : public LoggerSink {
 public:
  static std::unique_ptr<LoggerSink> create(const std::string& file) {
    auto sink = std::unique_ptr<LoggerSink>(new FileSink(file));
#if VLOG_ENABLED
    sink->out(std::format("--- [{}] vlog::FileSink started ---\n",
                          std::chrono::system_clock::now()),
              true);
#endif
    return sink;
  }
  virtual ~FileSink() override { log_.flush(); }

  LoggerSink& out(std::string_view string, bool flush = false) override {
    if (!log_.good()) {
      log_.open(file_, std::ofstream::out | std::ofstream::trunc);
    }
    if (log_.good()) {
      log_.write(string.data(), string.size());
      if (flush) {
        log_.flush();
      }
    }
    return *this;
  }

 private:
  FileSink(const std::string& file) : file_(file) {}

 private:
  std::string file_;
  std::ofstream log_;
};

constexpr std::array<std::string, 5> kVerbosityStrings = {"E", "W", "I", "D",
                                                          "T"};

class Logger {
 public:
  enum class Verbosity : uint8_t {
    ERROR = 0,
    WARNING,
    INFO,
    DEBUG,
    TRACE,
    LAST,
  };

  struct Configuration {
    enum class Source { NONE = 0, FILE, FUNCTION, PRETTY_FUNCTION } show_source;
    Verbosity level{Verbosity::ERROR};
  };

  static constexpr Configuration kDefaultConfiguration = {
      .show_source = Configuration::Source::FUNCTION,
      .level = Verbosity::INFO,
  };

  static Logger& instance() {
    static Logger logger;
    return logger;
  }

  Logger& set_verbosity(Verbosity level) {
    auto& logger = Logger::instance();
    logger.configuration_.level = level;
    return logger;
  }

  static Logger& init(
      const Configuration& configuration = kDefaultConfiguration) {
    auto& logger = Logger::instance();
#if VLOG_ENABLED
    logger.configuration_ = configuration;
    logger.add_sink(ConsoleSink::create());
#endif
    return logger;
  }

  Logger& add_sink(std::unique_ptr<LoggerSink> sink) {
    auto& logger = Logger::instance();
#if VLOG_ENABLED
    logger.sinks_.emplace_back(std::move(sink));
#endif
    return logger;
  }

  template <typename... Args>
  auto print(Verbosity level, std::string_view file, std::string_view function,
             std::string_view pretty_function, int line,
             std::format_string<Args...> fmt, Args&&... args) {
    if (level > configuration_.level) {
      return;
    }
    const auto timestamp =
        std::format("[{}] ", std::chrono::system_clock::now());
    const auto level_str =
        std::format("<{}> ", kVerbosityStrings[static_cast<uint8_t>(level)]);

    std::string src_str;
    if (configuration_.show_source != Configuration::Source::NONE) {
      src_str = std::format(
          "{}:{} : ",
          configuration_.show_source == Configuration::Source::FILE ? file
          : configuration_.show_source == Configuration::Source::FUNCTION
              ? function
              : pretty_function,
          line);
    }
    const auto str = std::format(fmt, std::forward<Args>(args)...);
    for (auto& s : sinks_) {
      s->out(timestamp);
      s->out(level_str);
      if (configuration_.show_source != Configuration::Source::NONE) {
        s->out(src_str);
      }
      s->out(str);
      s->endl();
    }
  }

 protected:
  Logger() = default;
  virtual ~Logger() = default;

 private:
  std::list<std::unique_ptr<LoggerSink>> sinks_;
  Configuration configuration_;
};

#if VLOG_ENABLED

#define LOGE(fmt, ...)                                                       \
  do {                                                                       \
    vlog::Logger::instance().print(vlog::Logger::Verbosity::ERROR, __FILE__, \
                                   __func__, __PRETTY_FUNCTION__, __LINE__,  \
                                   fmt, ##__VA_ARGS__);                      \
  } while (0)

#define LOGW(fmt, ...)                                                         \
  do {                                                                         \
    vlog::Logger::instance().print(vlog::Logger::Verbosity::WARNING, __FILE__, \
                                   __func__, __PRETTY_FUNCTION__, __LINE__,    \
                                   fmt, ##__VA_ARGS__);                        \
  } while (0)

#define LOGI(fmt, ...)                                                      \
  do {                                                                      \
    vlog::Logger::instance().print(vlog::Logger::Verbosity::INFO, __FILE__, \
                                   __func__, __PRETTY_FUNCTION__, __LINE__, \
                                   fmt, ##__VA_ARGS__);                     \
  } while (0)

#define LOGD(fmt, ...)                                                       \
  do {                                                                       \
    vlog::Logger::instance().print(vlog::Logger::Verbosity::DEBUG, __FILE__, \
                                   __func__, __PRETTY_FUNCTION__, __LINE__,  \
                                   fmt, ##__VA_ARGS__);                      \
  } while (0)

#define LOGT(fmt, ...)                                                       \
  do {                                                                       \
    vlog::Logger::instance().print(vlog::Logger::Verbosity::TRACE, __FILE__, \
                                   __func__, __PRETTY_FUNCTION__, __LINE__,  \
                                   fmt, ##__VA_ARGS__);                      \
  } while (0)

#else

#define LOGE(fmt, ...)
#define LOGW(fmt, ...)
#define LOGI(fmt, ...)
#define LOGD(fmt, ...)
#define LOGT(fmt, ...)

#endif

}  // namespace vlog