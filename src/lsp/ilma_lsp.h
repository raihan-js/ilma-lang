#ifndef ILMA_LSP_H
#define ILMA_LSP_H

/* ILMA Language Server Protocol implementation */

/* Read a JSON-RPC message from stdin (Content-Length header + body) */
char* lsp_read_message(void);

/* Send a JSON-RPC response to stdout */
void lsp_send_response(int id, const char* result_json);

/* Send a JSON-RPC notification (no id) */
void lsp_send_notification(const char* method, const char* params_json);

/* Send diagnostics for a file */
void lsp_send_diagnostics(const char* uri, const char* diagnostics_json);

/* Process a single message — returns 0 to continue, 1 to exit */
int lsp_process_message(const char* message);

/* Run the LSP event loop */
void lsp_run(void);

/* Get hover documentation for a keyword */
const char* lsp_get_keyword_docs(const char* word);

#endif
