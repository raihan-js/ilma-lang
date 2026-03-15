#include "number.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

/* ── Helper: extract a number from ILMA_WHOLE or ILMA_DECIMAL ── */

static double to_num(IlmaValue v) {
    if (v.type == ILMA_WHOLE)   return (double)v.as_whole;
    if (v.type == ILMA_DECIMAL) return v.as_decimal;
    return 0.0;
}

/* ── ilma_number_sqrt ─────────────────────────────────────────── */

IlmaValue ilma_number_sqrt(IlmaValue n) {
    return ilma_decimal(sqrt(to_num(n)));
}

/* ── ilma_number_abs ──────────────────────────────────────────── */

IlmaValue ilma_number_abs(IlmaValue n) {
    return ilma_decimal(fabs(to_num(n)));
}

/* ── ilma_number_floor ────────────────────────────────────────── */

IlmaValue ilma_number_floor(IlmaValue n) {
    return ilma_whole((int64_t)floor(to_num(n)));
}

/* ── ilma_number_ceil ─────────────────────────────────────────── */

IlmaValue ilma_number_ceil(IlmaValue n) {
    return ilma_whole((int64_t)ceil(to_num(n)));
}

/* ── ilma_number_round ────────────────────────────────────────── */

IlmaValue ilma_number_round(IlmaValue n) {
    return ilma_whole((int64_t)round(to_num(n)));
}

/* ── ilma_number_power ────────────────────────────────────────── */

IlmaValue ilma_number_power(IlmaValue base, IlmaValue exp) {
    return ilma_decimal(pow(to_num(base), to_num(exp)));
}

/* ── ilma_number_random ───────────────────────────────────────── */

IlmaValue ilma_number_random(IlmaValue min, IlmaValue max) {
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    int64_t lo = (int64_t)to_num(min);
    int64_t hi = (int64_t)to_num(max);

    if (hi < lo) {
        int64_t tmp = lo;
        lo = hi;
        hi = tmp;
    }

    int64_t range = hi - lo + 1;
    int64_t result = lo + (int64_t)(rand() % range);
    return ilma_whole(result);
}

/* ── ilma_number_is_prime ─────────────────────────────────────── */

IlmaValue ilma_number_is_prime(IlmaValue n) {
    int64_t num = (int64_t)to_num(n);

    if (num < 2) return ilma_no();
    if (num == 2) return ilma_yes();
    if (num % 2 == 0) return ilma_no();

    int64_t limit = (int64_t)sqrt((double)num);
    for (int64_t i = 3; i <= limit; i += 2) {
        if (num % i == 0) return ilma_no();
    }

    return ilma_yes();
}

/* ── ilma_number_fibonacci ────────────────────────────────────── */

IlmaValue ilma_number_fibonacci(IlmaValue n) {
    int64_t num = (int64_t)to_num(n);

    if (num <= 0) return ilma_whole(0);
    if (num == 1) return ilma_whole(1);

    int64_t a = 0, b = 1;
    for (int64_t i = 2; i <= num; i++) {
        int64_t tmp = a + b;
        a = b;
        b = tmp;
    }

    return ilma_whole(b);
}

/* ── ilma_number_to_binary ────────────────────────────────────── */

IlmaValue ilma_number_to_binary(IlmaValue n) {
    int64_t num = (int64_t)to_num(n);

    if (num == 0) return ilma_text("0");

    /* Handle negative numbers: work with absolute value */
    int negative = 0;
    if (num < 0) {
        negative = 1;
        num = -num;
    }

    char buf[128];
    int pos = sizeof(buf) - 1;
    buf[pos] = '\0';

    while (num > 0) {
        pos--;
        buf[pos] = (num & 1) ? '1' : '0';
        num >>= 1;
    }

    if (negative) {
        pos--;
        buf[pos] = '-';
    }

    return ilma_text(&buf[pos]);
}

/* ── ilma_number_to_hex ───────────────────────────────────────── */

IlmaValue ilma_number_to_hex(IlmaValue n) {
    int64_t num = (int64_t)to_num(n);

    char buf[32];
    if (num < 0) {
        snprintf(buf, sizeof(buf), "-%llX", (unsigned long long)(-num));
    } else {
        snprintf(buf, sizeof(buf), "%llX", (unsigned long long)num);
    }

    return ilma_text(buf);
}

/* ── ilma_number_pi ───────────────────────────────────────────── */

IlmaValue ilma_number_pi(void) {
    return ilma_decimal(3.14159265358979);
}

/* ── ilma_number_e ────────────────────────────────────────────── */

IlmaValue ilma_number_e(void) {
    return ilma_decimal(2.71828182845905);
}
