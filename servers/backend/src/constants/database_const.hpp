#pragma once

namespace database {

inline const char* CONNECTION_INFO =
    "host=ai_stream2_postgresql port=5432 dbname=ai_stream2 user=ai_stream2 password=ai_stream2";
inline const char* MIGRATIONS_DIR = "/app/src/db/migrations";
inline const int CONNECT_ATTEMPTS = 10;
inline const int CONNECT_DELAY_SECONDS = 1;

}
