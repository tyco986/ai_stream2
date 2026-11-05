#ifndef API_ERROR_HPP
#define API_ERROR_HPP

#include <stdexcept>
#include <string>

class ApiError : public std::runtime_error {
 public:
  ApiError(std::string message, int status_code)
      : std::runtime_error(message), status_code_(status_code) {}

  int status_code() const { return status_code_; }

 private:
  int status_code_;
};

#endif
