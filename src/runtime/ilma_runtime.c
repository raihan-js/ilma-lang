#include "ilma_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>

/* ── Constructor helpers ──────────────────────────────── */

IlmaValue ilma_whole(int64_t n) {
    IlmaValue v;
    v.type = ILMA_WHOLE;
    v.as_whole = n;
    return v;
}

IlmaValue ilma_decimal(double d) {
    IlmaValue v;
    v.type = ILMA_DECIMAL;
    v.as_decimal = d;
    return v;
}

IlmaValue ilma_text(const char* s) {
    IlmaValue v;
    v.type = ILMA_TEXT;
    v.as_text = strdup(s);
    return v;
}

IlmaValue ilma_yes(void) {
    IlmaValue v;
    v.type = ILMA_TRUTH;
    v.as_truth = 1;
    return v;
}

IlmaValue ilma_no(void) {
    IlmaValue v;
    v.type = ILMA_TRUTH;
    v.as_truth = 0;
    return v;
}

IlmaValue ilma_empty_val(void) {
    IlmaValue v;
    v.type = ILMA_EMPTY;
    v.as_whole = 0;
    return v;
}

/* ── Conversion to text ───────────────────────────────── */

char* ilma_to_string(IlmaValue v) {
    char buf[256];
    switch (v.type) {
        case ILMA_WHOLE:
            snprintf(buf, sizeof(buf), "%ld", (long)v.as_whole);
            return strdup(buf);
        case ILMA_DECIMAL: {
            snprintf(buf, sizeof(buf), "%g", v.as_decimal);
            return strdup(buf);
        }
        case ILMA_TEXT:
            return strdup(v.as_text);
        case ILMA_TRUTH:
            return strdup(v.as_truth ? "yes" : "no");
        case ILMA_EMPTY:
            return strdup("empty");
        case ILMA_BAG: {
            if (!v.as_bag || v.as_bag->count == 0) return strdup("bag[]");
            /* Build "bag[item1, item2, ...]" */
            size_t total = 8; /* "bag[" + "]\0" + safety */
            char** parts = malloc(sizeof(char*) * v.as_bag->count);
            for (int i = 0; i < v.as_bag->count; i++) {
                parts[i] = ilma_to_string(v.as_bag->items[i]);
                total += strlen(parts[i]) + 2;
            }
            char* result = malloc(total);
            strcpy(result, "bag[");
            for (int i = 0; i < v.as_bag->count; i++) {
                if (i > 0) strcat(result, ", ");
                strcat(result, parts[i]);
                free(parts[i]);
            }
            strcat(result, "]");
            free(parts);
            return result;
        }
        case ILMA_NOTEBOOK: {
            if (!v.as_notebook || v.as_notebook->count == 0) return strdup("notebook[]");
            size_t total = 16;
            /* Collect used entries */
            char* result = malloc(1024);
            strcpy(result, "notebook[");
            int first = 1;
            for (int i = 0; i < v.as_notebook->capacity; i++) {
                if (!v.as_notebook->entries[i].used) continue;
                if (!first) strcat(result, ", ");
                first = 0;
                strcat(result, v.as_notebook->entries[i].key);
                strcat(result, ": ");
                char* val = ilma_to_string(v.as_notebook->entries[i].value);
                strcat(result, val);
                free(val);
                (void)total;
            }
            strcat(result, "]");
            return result;
        }
        case ILMA_OBJECT:
            return strdup("[object]");
        default:
            return strdup("???");
    }
}

/* ── Type checking ────────────────────────────────────── */

int ilma_is_truthy(IlmaValue v) {
    switch (v.type) {
        case ILMA_WHOLE:    return v.as_whole != 0;
        case ILMA_DECIMAL:  return v.as_decimal != 0.0;
        case ILMA_TEXT:     return v.as_text != NULL && v.as_text[0] != '\0';
        case ILMA_TRUTH:    return v.as_truth;
        case ILMA_EMPTY:    return 0;
        case ILMA_BAG:      return v.as_bag != NULL && v.as_bag->count > 0;
        case ILMA_NOTEBOOK: return v.as_notebook != NULL && v.as_notebook->count > 0;
        case ILMA_OBJECT:   return v.as_object != NULL;
        default:            return 0;
    }
}

int ilma_is_empty(IlmaValue v) {
    return v.type == ILMA_EMPTY;
}

/* ── Promote to decimal if needed ─────────────────────── */

static double to_num(IlmaValue v) {
    if (v.type == ILMA_WHOLE) return (double)v.as_whole;
    if (v.type == ILMA_DECIMAL) return v.as_decimal;
    return 0.0;
}

static int either_decimal(IlmaValue a, IlmaValue b) {
    return a.type == ILMA_DECIMAL || b.type == ILMA_DECIMAL;
}

/* ── Arithmetic ───────────────────────────────────────── */

IlmaValue ilma_add(IlmaValue a, IlmaValue b) {
    /* String concatenation */
    if (a.type == ILMA_TEXT || b.type == ILMA_TEXT) {
        return ilma_concat(a, b);
    }
    if (either_decimal(a, b)) {
        return ilma_decimal(to_num(a) + to_num(b));
    }
    return ilma_whole(a.as_whole + b.as_whole);
}

IlmaValue ilma_sub(IlmaValue a, IlmaValue b) {
    if (either_decimal(a, b)) {
        return ilma_decimal(to_num(a) - to_num(b));
    }
    return ilma_whole(a.as_whole - b.as_whole);
}

IlmaValue ilma_mul(IlmaValue a, IlmaValue b) {
    if (either_decimal(a, b)) {
        return ilma_decimal(to_num(a) * to_num(b));
    }
    return ilma_whole(a.as_whole * b.as_whole);
}

IlmaValue ilma_div(IlmaValue a, IlmaValue b) {
    double bval = to_num(b);
    if (bval == 0.0) {
        fprintf(stderr, "Oops! You tried to divide by zero. That's not possible — "
                        "you can't split something into zero groups!\n");
        exit(1);
    }
    if (either_decimal(a, b)) {
        return ilma_decimal(to_num(a) / bval);
    }
    return ilma_whole(a.as_whole / b.as_whole);
}

IlmaValue ilma_mod(IlmaValue a, IlmaValue b) {
    if (b.as_whole == 0) {
        fprintf(stderr, "Oops! You tried to find the remainder when dividing by zero.\n");
        exit(1);
    }
    return ilma_whole(a.as_whole % b.as_whole);
}

IlmaValue ilma_neg(IlmaValue a) {
    if (a.type == ILMA_DECIMAL) return ilma_decimal(-a.as_decimal);
    return ilma_whole(-a.as_whole);
}

/* ── Comparison ───────────────────────────────────────── */

IlmaValue ilma_eq(IlmaValue a, IlmaValue b) {
    if (a.type == ILMA_TEXT && b.type == ILMA_TEXT) {
        return a.as_text && b.as_text && strcmp(a.as_text, b.as_text) == 0 ? ilma_yes() : ilma_no();
    }
    if (a.type == ILMA_TRUTH && b.type == ILMA_TRUTH) {
        return a.as_truth == b.as_truth ? ilma_yes() : ilma_no();
    }
    if (a.type == ILMA_EMPTY && b.type == ILMA_EMPTY) {
        return ilma_yes();
    }
    if (a.type == ILMA_EMPTY || b.type == ILMA_EMPTY) {
        return ilma_no();
    }
    return to_num(a) == to_num(b) ? ilma_yes() : ilma_no();
}

IlmaValue ilma_neq(IlmaValue a, IlmaValue b) {
    IlmaValue eq = ilma_eq(a, b);
    return eq.as_truth ? ilma_no() : ilma_yes();
}

IlmaValue ilma_lt(IlmaValue a, IlmaValue b) {
    return to_num(a) < to_num(b) ? ilma_yes() : ilma_no();
}

IlmaValue ilma_gt(IlmaValue a, IlmaValue b) {
    return to_num(a) > to_num(b) ? ilma_yes() : ilma_no();
}

IlmaValue ilma_leq(IlmaValue a, IlmaValue b) {
    return to_num(a) <= to_num(b) ? ilma_yes() : ilma_no();
}

IlmaValue ilma_geq(IlmaValue a, IlmaValue b) {
    return to_num(a) >= to_num(b) ? ilma_yes() : ilma_no();
}

/* ── Logic ────────────────────────────────────────────── */

IlmaValue ilma_and(IlmaValue a, IlmaValue b) {
    return (ilma_is_truthy(a) && ilma_is_truthy(b)) ? ilma_yes() : ilma_no();
}

IlmaValue ilma_or(IlmaValue a, IlmaValue b) {
    return (ilma_is_truthy(a) || ilma_is_truthy(b)) ? ilma_yes() : ilma_no();
}

IlmaValue ilma_not(IlmaValue a) {
    return ilma_is_truthy(a) ? ilma_no() : ilma_yes();
}

/* ── Output ───────────────────────────────────────────── */

void ilma_say(IlmaValue v) {
    char* s = ilma_to_string(v);
    printf("%s\n", s);
    free(s);
}

/* ── Input ────────────────────────────────────────────── */

IlmaValue ilma_ask(IlmaValue prompt) {
    char* s = ilma_to_string(prompt);
    printf("%s", s);
    fflush(stdout);
    free(s);

    char buf[1024];
    if (fgets(buf, sizeof(buf), stdin)) {
        /* Remove trailing newline */
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        return ilma_text(buf);
    }
    return ilma_text("");
}

/* ── Error handling ───────────────────────────────────── */

IlmaErrorStack ilma_error_stack = { .depth = 0 };

void ilma_shout(const char* message, int line) {
    if (ilma_error_stack.depth > 0) {
        /* Inside a try block — jump to the when wrong handler */
        int idx = ilma_error_stack.depth - 1;
        ilma_error_stack.error_msg[idx] = strdup(message);
        longjmp(ilma_error_stack.buf[idx], 1);
    }
    /* No try block — fatal error */
    fprintf(stderr, "Something went wrong on line %d: %s\n", line, message);
    exit(1);
}

/* ── String concat ────────────────────────────────────── */

IlmaValue ilma_concat(IlmaValue a, IlmaValue b) {
    char* sa = ilma_to_string(a);
    char* sb = ilma_to_string(b);
    size_t len = strlen(sa) + strlen(sb) + 1;
    char* result = malloc(len);
    strcpy(result, sa);
    strcat(result, sb);
    free(sa);
    free(sb);
    IlmaValue v;
    v.type = ILMA_TEXT;
    v.as_text = result;
    return v;
}

/* ── Bag operations ───────────────────────────────────── */

IlmaValue ilma_bag_new(void) {
    IlmaBag* bag = calloc(1, sizeof(IlmaBag));
    bag->capacity = 8;
    bag->items = malloc(sizeof(IlmaValue) * bag->capacity);
    bag->count = 0;
    IlmaValue v;
    v.type = ILMA_BAG;
    v.as_bag = bag;
    return v;
}

void ilma_bag_add(IlmaBag* bag, IlmaValue item) {
    if (bag->count >= bag->capacity) {
        bag->capacity *= 2;
        bag->items = realloc(bag->items, sizeof(IlmaValue) * bag->capacity);
    }
    bag->items[bag->count++] = item;
}

IlmaValue ilma_bag_get(IlmaBag* bag, int index) {
    if (index < 0 || index >= bag->count) {
        fprintf(stderr, "Oops! You tried to get item number %d from a bag "
                        "that only has %d items.\n", index, bag->count);
        exit(1);
    }
    return bag->items[index];
}

int ilma_bag_size(IlmaBag* bag) {
    return bag->count;
}

IlmaValue ilma_bag_size_val(IlmaBag* bag) {
    return ilma_whole(bag->count);
}

void ilma_bag_remove(IlmaBag* bag, IlmaValue item) {
    char* item_str = ilma_to_string(item);
    for (int i = 0; i < bag->count; i++) {
        char* elem_str = ilma_to_string(bag->items[i]);
        int match = strcmp(item_str, elem_str) == 0 &&
                    bag->items[i].type == item.type;
        free(elem_str);
        if (match) {
            /* Shift remaining elements left */
            for (int j = i; j < bag->count - 1; j++) {
                bag->items[j] = bag->items[j + 1];
            }
            bag->count--;
            free(item_str);
            return;
        }
    }
    free(item_str);
}

static int ilma_value_compare(const void* a, const void* b) {
    const IlmaValue* va = (const IlmaValue*)a;
    const IlmaValue* vb = (const IlmaValue*)b;

    /* Numbers compare numerically */
    if ((va->type == ILMA_WHOLE || va->type == ILMA_DECIMAL) &&
        (vb->type == ILMA_WHOLE || vb->type == ILMA_DECIMAL)) {
        double da = va->type == ILMA_WHOLE ? (double)va->as_whole : va->as_decimal;
        double db = vb->type == ILMA_WHOLE ? (double)vb->as_whole : vb->as_decimal;
        if (da < db) return -1;
        if (da > db) return 1;
        return 0;
    }

    /* Text compares lexicographically */
    if (va->type == ILMA_TEXT && vb->type == ILMA_TEXT) {
        return strcmp(va->as_text, vb->as_text);
    }

    /* Different types: compare by type tag */
    return (int)va->type - (int)vb->type;
}

IlmaValue ilma_bag_sorted(IlmaBag* bag) {
    IlmaValue new_bag = ilma_bag_new();
    for (int i = 0; i < bag->count; i++) {
        ilma_bag_add(new_bag.as_bag, bag->items[i]);
    }
    qsort(new_bag.as_bag->items, new_bag.as_bag->count, sizeof(IlmaValue), ilma_value_compare);
    return new_bag;
}

/* ── Higher-order bag operations ─────────────────────── */

IlmaValue ilma_bag_map(IlmaBag* bag, IlmaMapFn fn) {
    IlmaValue result = ilma_bag_new();
    for (int i = 0; i < bag->count; i++) {
        ilma_bag_add(result.as_bag, fn(bag->items[i]));
    }
    return result;
}

IlmaValue ilma_bag_filter(IlmaBag* bag, IlmaMapFn fn) {
    IlmaValue result = ilma_bag_new();
    for (int i = 0; i < bag->count; i++) {
        IlmaValue r = fn(bag->items[i]);
        if (ilma_is_truthy(r)) {
            ilma_bag_add(result.as_bag, bag->items[i]);
        }
    }
    return result;
}

void ilma_bag_each(IlmaBag* bag, IlmaMapFn fn) {
    for (int i = 0; i < bag->count; i++) {
        fn(bag->items[i]);
    }
}

/* ── Notebook operations ──────────────────────────────── */

IlmaValue ilma_notebook_new(void) {
    IlmaBook* book = calloc(1, sizeof(IlmaBook));
    book->capacity = 16;
    book->entries = calloc(book->capacity, sizeof(IlmaBookEntry));
    book->count = 0;
    IlmaValue v;
    v.type = ILMA_NOTEBOOK;
    v.as_notebook = book;
    return v;
}

static unsigned int hash_string(const char* key, int capacity) {
    unsigned int hash = 5381;
    while (*key) {
        hash = ((hash << 5) + hash) + (unsigned char)*key;
        key++;
    }
    return hash % capacity;
}

void ilma_notebook_set(IlmaBook* book, const char* key, IlmaValue value) {
    unsigned int idx = hash_string(key, book->capacity);
    /* Linear probing */
    for (int i = 0; i < book->capacity; i++) {
        unsigned int pos = (idx + i) % book->capacity;
        if (!book->entries[pos].used) {
            book->entries[pos].key = strdup(key);
            book->entries[pos].value = value;
            book->entries[pos].used = 1;
            book->count++;
            return;
        }
        if (strcmp(book->entries[pos].key, key) == 0) {
            book->entries[pos].value = value;
            return;
        }
    }
    fprintf(stderr, "Oops! The notebook is full. This shouldn't happen!\n");
    exit(1);
}

/* ── Object operations ────────────────────────────────── */

IlmaValue ilma_obj_new(const char* blueprint_name) {
    IlmaObj* obj = calloc(1, sizeof(IlmaObj));
    obj->blueprint = blueprint_name;
    /* Allocate a fresh notebook for fields */
    IlmaBook* book = calloc(1, sizeof(IlmaBook));
    book->capacity = 16;
    book->entries = calloc(book->capacity, sizeof(IlmaBookEntry));
    book->count = 0;
    obj->fields = book;
    IlmaValue v;
    v.type = ILMA_OBJECT;
    v.as_object = obj;
    return v;
}

void ilma_obj_set(IlmaObj* obj, const char* field, IlmaValue value) {
    ilma_notebook_set(obj->fields, field, value);
}

IlmaValue ilma_obj_get(IlmaObj* obj, const char* field) {
    return ilma_notebook_get(obj->fields, field);
}

const char* ilma_obj_blueprint(IlmaObj* obj) {
    return obj->blueprint;
}

/* ── Text operations ──────────────────────────────────── */

IlmaValue ilma_text_join(IlmaValue separator, IlmaBag* bag) {
    if (bag->count == 0) return ilma_text("");
    char* sep = ilma_to_string(separator);
    size_t sep_len = strlen(sep);

    /* Calculate total length */
    char** parts = malloc(sizeof(char*) * bag->count);
    size_t total = 0;
    for (int i = 0; i < bag->count; i++) {
        parts[i] = ilma_to_string(bag->items[i]);
        total += strlen(parts[i]);
        if (i > 0) total += sep_len;
    }

    char* result = malloc(total + 1);
    result[0] = '\0';
    for (int i = 0; i < bag->count; i++) {
        if (i > 0) strcat(result, sep);
        strcat(result, parts[i]);
        free(parts[i]);
    }
    free(parts);
    free(sep);

    IlmaValue v;
    v.type = ILMA_TEXT;
    v.as_text = result;
    return v;
}

IlmaValue ilma_text_length(IlmaValue text) {
    if (text.type != ILMA_TEXT) return ilma_whole(0);
    return ilma_whole((int64_t)strlen(text.as_text));
}

IlmaValue ilma_text_upper(IlmaValue text) {
    if (text.type != ILMA_TEXT) return ilma_text("");
    char* s = strdup(text.as_text);
    for (size_t i = 0; s[i]; i++) {
        if (s[i] >= 'a' && s[i] <= 'z') s[i] -= 32;
    }
    IlmaValue v;
    v.type = ILMA_TEXT;
    v.as_text = s;
    return v;
}

IlmaValue ilma_text_lower(IlmaValue text) {
    if (text.type != ILMA_TEXT) return ilma_text("");
    char* s = strdup(text.as_text);
    for (size_t i = 0; s[i]; i++) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] += 32;
    }
    IlmaValue v;
    v.type = ILMA_TEXT;
    v.as_text = s;
    return v;
}

IlmaValue ilma_text_contains(IlmaValue text, IlmaValue search) {
    if (text.type != ILMA_TEXT || search.type != ILMA_TEXT) return ilma_no();
    return strstr(text.as_text, search.as_text) ? ilma_yes() : ilma_no();
}

IlmaValue ilma_text_slice(IlmaValue text, IlmaValue start, IlmaValue end) {
    if (text.type != ILMA_TEXT) return ilma_text("");
    int64_t len = (int64_t)strlen(text.as_text);
    int64_t s = start.as_whole;
    int64_t e = end.as_whole;
    if (s < 0) s = 0;
    if (e > len) e = len;
    if (s >= e) return ilma_text("");
    size_t slice_len = (size_t)(e - s);
    char* result = malloc(slice_len + 1);
    memcpy(result, text.as_text + s, slice_len);
    result[slice_len] = '\0';
    IlmaValue v;
    v.type = ILMA_TEXT;
    v.as_text = result;
    return v;
}

/* ── Notebook operations ──────────────────────────────── */

IlmaValue ilma_notebook_get(IlmaBook* book, const char* key) {
    unsigned int idx = hash_string(key, book->capacity);
    for (int i = 0; i < book->capacity; i++) {
        unsigned int pos = (idx + i) % book->capacity;
        if (!book->entries[pos].used) {
            return ilma_empty_val();
        }
        if (strcmp(book->entries[pos].key, key) == 0) {
            return book->entries[pos].value;
        }
    }
    return ilma_empty_val();
}

IlmaValue ilma_notebook_get_val(IlmaBook* book, IlmaValue key) {
    char* key_str = ilma_to_string(key);
    IlmaValue result = ilma_notebook_get(book, key_str);
    free(key_str);
    return result;
}

IlmaValue ilma_notebook_keys(IlmaBook* book) {
    IlmaValue keys = ilma_bag_new();
    for (int i = 0; i < book->capacity; i++) {
        if (book->entries[i].used) {
            ilma_bag_add(keys.as_bag, ilma_text(book->entries[i].key));
        }
    }
    return keys;
}

int ilma_notebook_size(IlmaBook* book) {
    return book->count;
}

/* ── File I/O ────────────────────────────────────────── */

IlmaValue ilma_read_file(IlmaValue path) {
    if (path.type != ILMA_TEXT || !path.as_text) {
        ilma_shout("read_file expects a text path", 0);
        return ilma_empty_val();
    }
    FILE* f = fopen(path.as_text, "rb");
    if (!f) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Cannot read file: %s", path.as_text);
        ilma_shout(msg, 0);
        return ilma_empty_val();
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(size + 1);
    if (buf) {
        size_t read = fread(buf, 1, size, f);
        buf[read] = '\0';
    }
    fclose(f);
    IlmaValue result = ilma_text(buf);
    free(buf);
    return result;
}

IlmaValue ilma_write_file(IlmaValue path, IlmaValue content) {
    if (path.type != ILMA_TEXT || !path.as_text) {
        ilma_shout("write_file expects a text path", 0);
        return ilma_no();
    }
    const char* data = "";
    if (content.type == ILMA_TEXT && content.as_text) {
        data = content.as_text;
    } else {
        char* s = ilma_to_string(content);
        FILE* f = fopen(path.as_text, "w");
        if (!f) {
            char msg[512];
            snprintf(msg, sizeof(msg), "Cannot write file: %s", path.as_text);
            free(s);
            ilma_shout(msg, 0);
            return ilma_no();
        }
        fprintf(f, "%s", s);
        fclose(f);
        free(s);
        return ilma_yes();
    }
    FILE* f = fopen(path.as_text, "w");
    if (!f) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Cannot write file: %s", path.as_text);
        ilma_shout(msg, 0);
        return ilma_no();
    }
    fprintf(f, "%s", data);
    fclose(f);
    return ilma_yes();
}

IlmaValue ilma_file_exists(IlmaValue path) {
    if (path.type != ILMA_TEXT || !path.as_text) return ilma_no();
    FILE* f = fopen(path.as_text, "r");
    if (f) { fclose(f); return ilma_yes(); }
    return ilma_no();
}

/* ── System built-ins ────────────────────────────────── */

void ilma_print(IlmaValue v) {
    /* Like ilma_say but no newline */
    switch (v.type) {
        case ILMA_WHOLE:   printf("%" PRId64, v.as_whole); break;
        case ILMA_DECIMAL: printf("%g", v.as_decimal); break;
        case ILMA_TEXT:    printf("%s", v.as_text ? v.as_text : ""); break;
        case ILMA_TRUTH:   printf("%s", v.as_truth ? "yes" : "no"); break;
        case ILMA_EMPTY:   printf("empty"); break;
        default:           printf("[value]"); break;
    }
}

IlmaValue ilma_timestamp(void) {
    time_t t = time(NULL);
    return ilma_whole((int64_t)t);
}

void ilma_exit_program(IlmaValue code) {
    int c = 0;
    if (code.type == ILMA_WHOLE) c = (int)code.as_whole;
    exit(c);
}

IlmaValue ilma_env_get(IlmaValue name) {
    if (name.type != ILMA_TEXT || !name.as_text) return ilma_empty_val();
    const char* val = getenv(name.as_text);
    if (!val) return ilma_empty_val();
    return ilma_text(val);
}

/* Global argv storage for ilma_args_get */
static char** g_ilma_argv = NULL;
static int     g_ilma_argc = 0;

void ilma_set_args(int argc, char** argv) {
    g_ilma_argc = argc;
    g_ilma_argv = argv;
}

IlmaValue ilma_args_get(void) {
    IlmaValue bag = ilma_bag_new();
    for (int i = 0; i < g_ilma_argc; i++) {
        ilma_bag_add(bag.as_bag, ilma_text(g_ilma_argv[i]));
    }
    return bag;
}

void ilma_sleep_ms(IlmaValue ms) {
    int64_t msec = 0;
    if (ms.type == ILMA_WHOLE) msec = ms.as_whole;
    else if (ms.type == ILMA_DECIMAL) msec = (int64_t)ms.as_decimal;
    if (msec > 0) {
        struct timespec ts;
        ts.tv_sec = msec / 1000;
        ts.tv_nsec = (msec % 1000) * 1000000;
        nanosleep(&ts, NULL);
    }
}
