#ifndef ILMA_DOCGEN_H
#define ILMA_DOCGEN_H

/* Generate HTML documentation from an ILMA source file.
 * source: the source code string
 * filename: used for titles
 * Returns 0 on success, 1 on error. */
int ilma_doc_generate(const char* source, const char* filename, const char* output_path);

#endif
