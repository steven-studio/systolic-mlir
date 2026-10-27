#include <boost/assert/source_location.hpp>
#include <boost/throw_exception.hpp>

#include "llvm/Support/ErrorHandling.h"

#include <exception>

namespace boost {

[[noreturn]]
void throw_exception(std::exception const &error) {
  llvm::report_fatal_error(error.what());
}

[[noreturn]]
void throw_exception(std::exception const &error,
                     boost::source_location const &) {
  llvm::report_fatal_error(error.what());
}

} // namespace boost
