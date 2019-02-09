//http://zetcode.com/db/sqlitec/
#include <stdio.h>      // printf
#include <unistd.h>     // sleep
#include <sqlite3.h>    // SQLite header

int callback(void *NotUsed, int argc, char **argv, char **azColName);
int main()
{
    sqlite3 *db;        // database connection
    int rc;             // return code
    char *errmsg;       // pointer to an error string

    /*
     * open SQLite database file test.db
     * use ":memory:" to use an in-memory database
     */
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        printf("ERROR opening SQLite DB in memory: %s\n", sqlite3_errmsg(db));
        goto out;
    }
    printf("opened SQLite handle successfully.\n");
    /* use the database... */

    char *sql = "DROP TABLE IF EXISTS Files;"
                "CREATE TABLE Files(Id INT, Name TEXT, Content BLOB);"
                "INSERT INTO Files VALUES(1, 'file', 'dfgdsfgsdfgsdfgsdfgsdfgsdfg');";

    rc = sqlite3_exec(db, sql, 0, 0, &errmsg);

    if (rc != SQLITE_OK ) {

        fprintf(stderr, "SQL error: %s\n", errmsg);

        sqlite3_free(errmsg);
        sqlite3_close(db);

        return 1;
    }

    sql = "SELECT * FROM Files";

    rc = sqlite3_exec(db, sql, callback, 0, &errmsg);

    if (rc != SQLITE_OK ) {

        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", errmsg);

        sqlite3_free(errmsg);
        sqlite3_close(db);

        return 1;
    }

out:
    /*
     * close SQLite database
     */
    sqlite3_close(db);
    printf("database closed.\n");
}

int callback(void *NotUsed, int argc, char **argv,
                    char **azColName) {

    NotUsed = 0;

    for (int i = 0; i < argc; i++) {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");

    return 0;
}
