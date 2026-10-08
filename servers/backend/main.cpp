#include "constants/main.hpp"
#include "db/database.hpp"

#include <drogon/drogon.h>

int main() {
  Database database;
  database.migrate();
  drogon::app().addListener(main::HOST, main::PORT).run();
  return 0;
}
