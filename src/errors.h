#ifndef ILMA_ERRORS_H
#define ILMA_ERRORS_H

typedef struct {
    const char** source_lines;
    int          line_count;
    const char*  filename;
} SourceMap;

/*
 * Print a Rust-style error with line/column indicator.
 *
 * message  — the main error text  (required)
 * hint     — shown after the caret (may be NULL)
 * example  — code example block    (may be NULL)
 */
void ilma_error(SourceMap* map, int line, int col,
                const char* message, const char* hint,
                const char* example);

/*
 * Print a warning (yellow). Same signature as ilma_error.
 */
void ilma_warning(SourceMap* map, int line, int col,
                  const char* message, const char* hint);

/* Convenience: build a SourceMap from a source string.
   Caller must call sourcemap_free() when done. */
SourceMap sourcemap_build(const char* source, const char* filename);
void      sourcemap_free(SourceMap* map);

#endif /* ILMA_ERRORS_H */
