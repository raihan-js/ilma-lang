#ifndef ILMA_DB_H
#define ILMA_DB_H

#include "../../src/runtime/ilma_runtime.h"

/* SQLite integration for IlmaWeb */
/* Note: requires -lsqlite3 when linking */

IlmaValue ilma_db_open(IlmaValue path);
IlmaValue ilma_db_query(IlmaValue db, IlmaValue sql);
IlmaValue ilma_db_exec(IlmaValue db, IlmaValue sql);
IlmaValue ilma_db_close(IlmaValue db);

#endif
