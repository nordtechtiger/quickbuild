#ifndef FORMAT_H
#define FORMAT_H

#define GREEN "\033[32m"
#define RED "\033[31m"
#define CYAN "\033[36m"
#define RESET "\033[0m"
#define BRIGHT "\033[97m"
#define LOG_VERBOSE(msg)                                                       \
  if (m_setup.logging_level >= LoggingLevel::Verbose) {                        \
    std::cout << msg << std::endl;                                             \
  }
#define LOG_STANDARD(msg)                                                      \
  if (m_setup.logging_level >= LoggingLevel::Standard) {                       \
    std::cout << msg << std::endl;                                             \
  }
#define LOG_QUIET(msg)                                                         \
  if (m_setup.logging_level >= LoggingLevel::Quiet) {                          \
    std::cout << msg << std::endl;                                             \
  }

#endif
