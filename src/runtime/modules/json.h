#ifndef ILMA_JSON_H
#define ILMA_JSON_H

#include "ilma_runtime.h"

/* Parse a JSON string into ILMA values:
   object  -> notebook
   array   -> bag
   string  -> text
   number  -> whole or decimal
   true    -> yes
   false   -> no
   null    -> empty */
IlmaValue ilma_json_parse(IlmaValue json_text);

/* Convert an ILMA value to a JSON string */
IlmaValue ilma_json_stringify(IlmaValue value);

/* Pretty-print with indentation (indent_size spaces per level) */
IlmaValue ilma_json_stringify_pretty(IlmaValue value, IlmaValue indent_size);

#endif /* ILMA_JSON_H */
