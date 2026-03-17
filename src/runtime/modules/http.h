#ifndef ILMA_HTTP_H
#define ILMA_HTTP_H

#include "ilma_runtime.h"

/*
 * HTTP GET request.
 * Returns: notebook[status: 200, body: "...", headers: notebook[...]]
 * On error: shout "HTTP error: <reason>"
 */
IlmaValue ilma_http_get(IlmaValue url);

/*
 * HTTP POST request.
 * body: request body text
 * content_type: e.g. ilma_text("application/json")
 */
IlmaValue ilma_http_post(IlmaValue url, IlmaValue body, IlmaValue content_type);

/*
 * Convenience: GET then json.parse(body)
 */
IlmaValue ilma_http_get_json(IlmaValue url);

/*
 * Convenience: json.stringify(data) + POST + json.parse(response)
 */
IlmaValue ilma_http_post_json(IlmaValue url, IlmaValue data);

#endif /* ILMA_HTTP_H */
