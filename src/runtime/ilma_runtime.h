#ifndef ILMA_RUNTIME_H
#define ILMA_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

/* ── Type tags ─────────────────────────────────────────── */

typedef enum {
    ILMA_WHOLE,
    ILMA_DECIMAL,
    ILMA_TEXT,
    ILMA_TRUTH,
    ILMA_BAG,
    ILMA_NOTEBOOK,
    ILMA_OBJECT,
    ILMA_EMPTY,
} IlmaType;

/* Forward declarations */
typedef struct IlmaBag IlmaBag;
typedef struct IlmaBook IlmaBook;
typedef struct IlmaObj IlmaObj;
typedef struct IlmaValue IlmaValue;

/* ── The core value type ───────────────────────────────── */

typedef struct IlmaValue {
    IlmaType type;
    union {
        int64_t    as_whole;
        double     as_decimal;
        char*      as_text;
        int        as_truth;
        IlmaBag*   as_bag;
        IlmaBook*  as_notebook;
        IlmaObj*   as_object;
    };
} IlmaValue;

/* ── Bag (dynamic array) ──────────────────────────────── */

struct IlmaBag {
    IlmaValue* items;
    int        count;
    int        capacity;
};

/* ── Notebook (hash map) ──────────────────────────────── */

typedef struct {
    char*      key;
    IlmaValue  value;
    int        used;
} IlmaBookEntry;

struct IlmaBook {
    IlmaBookEntry* entries;
    int             capacity;
    int             count;
};

/* ── Object (blueprint instance) ─────────────────────── */

struct IlmaObj {
    IlmaBook* fields;      /* dynamic fields set via me.x = val */
    const char* blueprint;  /* name of the blueprint (for debugging) */
};

/* ── Constructor helpers ──────────────────────────────── */

IlmaValue ilma_whole(int64_t n);
IlmaValue ilma_decimal(double d);
IlmaValue ilma_text(const char* s);
IlmaValue ilma_yes(void);
IlmaValue ilma_no(void);
IlmaValue ilma_empty_val(void);

/* ── Arithmetic ───────────────────────────────────────── */

IlmaValue ilma_add(IlmaValue a, IlmaValue b);
IlmaValue ilma_sub(IlmaValue a, IlmaValue b);
IlmaValue ilma_mul(IlmaValue a, IlmaValue b);
IlmaValue ilma_div(IlmaValue a, IlmaValue b);
IlmaValue ilma_mod(IlmaValue a, IlmaValue b);
IlmaValue ilma_neg(IlmaValue a);

/* ── Comparison ───────────────────────────────────────── */

IlmaValue ilma_eq(IlmaValue a, IlmaValue b);
IlmaValue ilma_neq(IlmaValue a, IlmaValue b);
IlmaValue ilma_lt(IlmaValue a, IlmaValue b);
IlmaValue ilma_gt(IlmaValue a, IlmaValue b);
IlmaValue ilma_leq(IlmaValue a, IlmaValue b);
IlmaValue ilma_geq(IlmaValue a, IlmaValue b);

/* ── Logic ────────────────────────────────────────────── */

IlmaValue ilma_and(IlmaValue a, IlmaValue b);
IlmaValue ilma_or(IlmaValue a, IlmaValue b);
IlmaValue ilma_not(IlmaValue a);

/* ── Type checking ────────────────────────────────────── */

int ilma_is_truthy(IlmaValue v);
int ilma_is_empty(IlmaValue v);

/* ── Conversion to text ───────────────────────────────── */

char* ilma_to_string(IlmaValue v);

/* ── Output ───────────────────────────────────────────── */

void ilma_say(IlmaValue v);

/* ── Input ────────────────────────────────────────────── */

IlmaValue ilma_ask(IlmaValue prompt);

/* ── Error handling (try / when wrong / shout) ────────── */

/* Error handling stack for try/when wrong */
#define ILMA_MAX_TRY_DEPTH 64

typedef struct {
    jmp_buf  buf[ILMA_MAX_TRY_DEPTH];
    char*    error_msg[ILMA_MAX_TRY_DEPTH];
    int      depth;
} IlmaErrorStack;

extern IlmaErrorStack ilma_error_stack;

void ilma_shout(const char* message, int line);

/* ── Bag operations ───────────────────────────────────── */

IlmaValue ilma_bag_new(void);
void      ilma_bag_add(IlmaBag* bag, IlmaValue item);
IlmaValue ilma_bag_get(IlmaBag* bag, int index);
int       ilma_bag_size(IlmaBag* bag);
void      ilma_bag_remove(IlmaBag* bag, IlmaValue item);
IlmaValue ilma_bag_sorted(IlmaBag* bag);
IlmaValue ilma_bag_size_val(IlmaBag* bag);

/* ── Higher-order bag operations ─────────────────────── */

typedef IlmaValue (*IlmaMapFn)(IlmaValue);

IlmaValue ilma_bag_map(IlmaBag* bag, IlmaMapFn fn);
IlmaValue ilma_bag_filter(IlmaBag* bag, IlmaMapFn fn);
void      ilma_bag_each(IlmaBag* bag, IlmaMapFn fn);

/* ── Notebook operations ──────────────────────────────── */

IlmaValue ilma_notebook_new(void);
void      ilma_notebook_set(IlmaBook* book, const char* key, IlmaValue value);
IlmaValue ilma_notebook_get(IlmaBook* book, const char* key);
IlmaValue ilma_notebook_get_val(IlmaBook* book, IlmaValue key);
IlmaValue ilma_notebook_keys(IlmaBook* book);
int       ilma_notebook_size(IlmaBook* book);

/* ── Object operations ────────────────────────────────── */

IlmaValue ilma_obj_new(const char* blueprint_name);
void      ilma_obj_set(IlmaObj* obj, const char* field, IlmaValue value);
IlmaValue ilma_obj_get(IlmaObj* obj, const char* field);
const char* ilma_obj_blueprint(IlmaObj* obj);

/* ── String concat helper ─────────────────────────────── */

IlmaValue ilma_concat(IlmaValue a, IlmaValue b);

/* ── Text operations ─────────────────────────────────── */

IlmaValue ilma_text_join(IlmaValue separator, IlmaBag* bag);
IlmaValue ilma_text_length(IlmaValue text);
IlmaValue ilma_text_upper(IlmaValue text);
IlmaValue ilma_text_lower(IlmaValue text);
IlmaValue ilma_text_contains(IlmaValue text, IlmaValue search);
IlmaValue ilma_text_slice(IlmaValue text, IlmaValue start, IlmaValue end);

/* ── File I/O ────────────────────────────────────────── */

IlmaValue ilma_read_file(IlmaValue path);
IlmaValue ilma_write_file(IlmaValue path, IlmaValue content);
IlmaValue ilma_file_exists(IlmaValue path);

/* ── System built-ins ────────────────────────────────── */

void      ilma_print(IlmaValue v);           /* like say but no newline */
IlmaValue ilma_timestamp(void);              /* Unix timestamp */
void      ilma_exit_program(IlmaValue code); /* exit with code */
IlmaValue ilma_env_get(IlmaValue name);      /* get env var */
IlmaValue ilma_args_get(void);               /* command line args */
void      ilma_sleep_ms(IlmaValue ms);       /* sleep milliseconds */
void      ilma_set_args(int argc, char** argv);

#endif /* ILMA_RUNTIME_H */
