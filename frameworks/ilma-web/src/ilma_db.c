/*
 * ilma_db.c — SQLite wrapper for IlmaWeb
 *
 * Provides database access for ILMA web applications.
 * Compile with -DILMA_HAS_SQLITE -lsqlite3 to enable full SQLite support.
 * Without the flag, stub implementations return error messages.
 */

#include "ilma_db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef ILMA_HAS_SQLITE
#include <sqlite3.h>

/* ═════════════════════════════════════════════════════════════
 *  Full SQLite implementation
 * ═════════════════════════════════════════════════════════════ */

/*
 * ilma_db_open — Open a SQLite database
 *
 * Returns a notebook with _db_handle storing the sqlite3* pointer
 * (cast to int64_t) so it can be passed to other db functions.
 */
IlmaValue ilma_db_open(IlmaValue path) {
    if (path.type != ILMA_TEXT || !path.as_text) {
        return ilma_text("Error: db.open requires a text path");
    }

    sqlite3* db = NULL;
    int rc = sqlite3_open(path.as_text, &db);
    if (rc != SQLITE_OK) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Error: could not open database '%s': %s",
                 path.as_text, sqlite3_errmsg(db));
        if (db) sqlite3_close(db);
        return ilma_text(msg);
    }

    /* Store the handle in a notebook */
    IlmaValue result = ilma_notebook_new();
    ilma_notebook_set(result.as_notebook, "_db_handle",
                      ilma_whole((int64_t)(intptr_t)db));
    ilma_notebook_set(result.as_notebook, "path", path);
    ilma_notebook_set(result.as_notebook, "status", ilma_text("open"));

    return result;
}

/*
 * ilma_db_query — Execute a SELECT query
 *
 * Returns a bag of notebooks, one per row. Each notebook maps
 * column names to their values (text, whole, decimal, or empty).
 */
IlmaValue ilma_db_query(IlmaValue db, IlmaValue sql) {
    if (db.type != ILMA_NOTEBOOK || !db.as_notebook) {
        return ilma_text("Error: db.query requires a database notebook");
    }
    if (sql.type != ILMA_TEXT || !sql.as_text) {
        return ilma_text("Error: db.query requires a text SQL statement");
    }

    /* Retrieve the sqlite3* handle */
    IlmaValue handle_val = ilma_notebook_get(db.as_notebook, "_db_handle");
    if (handle_val.type != ILMA_WHOLE) {
        return ilma_text("Error: invalid database handle");
    }
    sqlite3* dbh = (sqlite3*)(intptr_t)handle_val.as_whole;

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(dbh, sql.as_text, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Error: SQL prepare failed: %s",
                 sqlite3_errmsg(dbh));
        return ilma_text(msg);
    }

    /* Build a bag of row notebooks */
    IlmaValue rows = ilma_bag_new();
    int col_count = sqlite3_column_count(stmt);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        IlmaValue row = ilma_notebook_new();

        for (int i = 0; i < col_count; i++) {
            const char* col_name = sqlite3_column_name(stmt, i);
            int col_type = sqlite3_column_type(stmt, i);

            IlmaValue val;
            switch (col_type) {
                case SQLITE_INTEGER:
                    val = ilma_whole(sqlite3_column_int64(stmt, i));
                    break;
                case SQLITE_FLOAT:
                    val = ilma_decimal(sqlite3_column_double(stmt, i));
                    break;
                case SQLITE_TEXT:
                    val = ilma_text((const char*)sqlite3_column_text(stmt, i));
                    break;
                case SQLITE_NULL:
                default:
                    val = ilma_empty_val();
                    break;
            }

            ilma_notebook_set(row.as_notebook, col_name, val);
        }

        ilma_bag_add(rows.as_bag, row);
    }

    sqlite3_finalize(stmt);
    return rows;
}

/*
 * ilma_db_exec — Execute an INSERT/UPDATE/DELETE statement
 *
 * Returns yes on success, no on failure.
 */
IlmaValue ilma_db_exec(IlmaValue db, IlmaValue sql) {
    if (db.type != ILMA_NOTEBOOK || !db.as_notebook) {
        return ilma_text("Error: db.exec requires a database notebook");
    }
    if (sql.type != ILMA_TEXT || !sql.as_text) {
        return ilma_text("Error: db.exec requires a text SQL statement");
    }

    /* Retrieve the sqlite3* handle */
    IlmaValue handle_val = ilma_notebook_get(db.as_notebook, "_db_handle");
    if (handle_val.type != ILMA_WHOLE) {
        return ilma_text("Error: invalid database handle");
    }
    sqlite3* dbh = (sqlite3*)(intptr_t)handle_val.as_whole;

    char* err_msg = NULL;
    int rc = sqlite3_exec(dbh, sql.as_text, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        if (err_msg) {
            char msg[512];
            snprintf(msg, sizeof(msg), "Error: SQL exec failed: %s", err_msg);
            sqlite3_free(err_msg);
            return ilma_no();
        }
        return ilma_no();
    }

    return ilma_yes();
}

/*
 * ilma_db_close — Close the SQLite database
 *
 * Returns yes on success, no on failure.
 */
IlmaValue ilma_db_close(IlmaValue db) {
    if (db.type != ILMA_NOTEBOOK || !db.as_notebook) {
        return ilma_text("Error: db.close requires a database notebook");
    }

    IlmaValue handle_val = ilma_notebook_get(db.as_notebook, "_db_handle");
    if (handle_val.type != ILMA_WHOLE) {
        return ilma_text("Error: invalid database handle");
    }
    sqlite3* dbh = (sqlite3*)(intptr_t)handle_val.as_whole;

    int rc = sqlite3_close(dbh);
    if (rc != SQLITE_OK) {
        return ilma_no();
    }

    /* Mark the handle as closed */
    ilma_notebook_set(db.as_notebook, "status", ilma_text("closed"));
    ilma_notebook_set(db.as_notebook, "_db_handle", ilma_whole(0));

    return ilma_yes();
}

#else /* !ILMA_HAS_SQLITE */

/* ═════════════════════════════════════════════════════════════
 *  Stub implementations — SQLite not available
 * ═════════════════════════════════════════════════════════════ */

IlmaValue ilma_db_open(IlmaValue path) {
    (void)path;
    return ilma_text("SQLite not available. Compile with -DILMA_HAS_SQLITE -lsqlite3");
}

IlmaValue ilma_db_query(IlmaValue db, IlmaValue sql) {
    (void)db;
    (void)sql;
    return ilma_text("SQLite not available. Compile with -DILMA_HAS_SQLITE -lsqlite3");
}

IlmaValue ilma_db_exec(IlmaValue db, IlmaValue sql) {
    (void)db;
    (void)sql;
    return ilma_text("SQLite not available. Compile with -DILMA_HAS_SQLITE -lsqlite3");
}

IlmaValue ilma_db_close(IlmaValue db) {
    (void)db;
    return ilma_text("SQLite not available. Compile with -DILMA_HAS_SQLITE -lsqlite3");
}

#endif /* ILMA_HAS_SQLITE */
