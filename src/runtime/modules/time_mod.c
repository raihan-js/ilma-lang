#include "time_mod.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long gregorian_to_jd(int year, int month, int day) {
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    long jd = day + (153 * m + 2) / 5 + 365 * (long)y + y / 4 - y / 100 + y / 400 - 32045;
    return jd;
}

static void parse_date(const char* str, int* year, int* month, int* day) {
    *year = 0;
    *month = 0;
    *day = 0;
    if (str != NULL) {
        sscanf(str, "%d-%d-%d", year, month, day);
    }
}

IlmaValue ilma_time_today_str(void) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", t);
    return ilma_text(buf);
}

IlmaValue ilma_time_to_hijri(IlmaValue gregorian_str) {
    static const char* hijri_months[12] = {
        "Muharram", "Safar", "Rabi al-Awwal", "Rabi al-Thani",
        "Jumada al-Awwal", "Jumada al-Thani", "Rajab", "Sha'ban",
        "Ramadan", "Shawwal", "Dhu al-Qi'dah", "Dhu al-Hijjah"
    };

    const char* str = "";
    if (gregorian_str.type == ILMA_TEXT && gregorian_str.as_text != NULL) {
        str = gregorian_str.as_text;
    }

    int year, month, day;
    parse_date(str, &year, &month, &day);

    long jd = gregorian_to_jd(year, month, day);

    /* Convert Julian Day to Hijri calendar */
    long l = jd - 1948440 + 10632;
    long n = (l - 1) / 10631;
    l = l - 10631 * n + 354;
    long j = ((10985 - l) / 5316) * ((50 * l) / 17719) + (l / 5670) * ((43 * l) / 15238);
    l = l - ((30 - j) / 15) * ((17719 * j) / 50) - (j / 16) * ((15238 * j) / 43) + 29;
    long hm = (24 * l) / 709;
    long hd = l - (709 * hm) / 24;
    long hy = 30 * n + j - 30;

    int month_index = (int)hm - 1;
    if (month_index < 0) month_index = 0;
    if (month_index > 11) month_index = 11;

    char result[128];
    snprintf(result, sizeof(result), "%ld %s %ld (Hijri)", hd, hijri_months[month_index], hy);
    return ilma_text(result);
}

IlmaValue ilma_time_days_between(IlmaValue date1_str, IlmaValue date2_str) {
    const char* s1 = "";
    const char* s2 = "";
    if (date1_str.type == ILMA_TEXT && date1_str.as_text != NULL) s1 = date1_str.as_text;
    if (date2_str.type == ILMA_TEXT && date2_str.as_text != NULL) s2 = date2_str.as_text;

    int y1, m1, d1, y2, m2, d2;
    parse_date(s1, &y1, &m1, &d1);
    parse_date(s2, &y2, &m2, &d2);

    long jd1 = gregorian_to_jd(y1, m1, d1);
    long jd2 = gregorian_to_jd(y2, m2, d2);

    long diff = jd1 - jd2;
    if (diff < 0) diff = -diff;

    return ilma_whole((int64_t)diff);
}
