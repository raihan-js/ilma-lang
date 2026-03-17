#ifndef ILMA_FORMATTER_H
#define ILMA_FORMATTER_H

/* Format the given source code string.
 * Returns newly allocated formatted string (caller must free).
 * If error, returns NULL. */
char* ilma_fmt_source(const char* source);

/* Format in-place: read file, format, write back.
 * Returns 0 on success, 1 on error. */
int ilma_fmt_file(const char* filename, int check_only);

#endif
