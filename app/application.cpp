#include "app/application_internal.h"

namespace velo::app {

Application::Application(HINSTANCE instance, CommandLineOptions options)
    : instance_(instance),
      options_(std::move(options)),
      configService_(velo::config::DefaultConfigRoot()),
      logger_(velo::diagnostics::DefaultLogRoot()),
      sessionRecovery_(velo::config::DefaultConfigRoot()) {}


}  // namespace velo::app

