#include "constants/i18n_const.hpp"
#include "constants/service_main.hpp"
#include "db/database.hpp"
#include "i18n/i18n.hpp"

#include <drogon/drogon.h>

int main() {
  Database database;
  database.migrate();
  I18n::load(i18n::DIRECTORY);
  drogon::app()
      .loadConfigFile(service_main::CONFIG_FILE)
      .registerPreHandlingAdvice(I18n::bind)
      .addListener(service_main::HOST, service_main::PORT)
      .run();
  return 0;
}
