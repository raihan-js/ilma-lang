#include "finance.h"
#include <math.h>

static double to_num(IlmaValue v) {
    if (v.type == ILMA_WHOLE) return (double)v.as_whole;
    if (v.type == ILMA_DECIMAL) return v.as_decimal;
    return 0.0;
}

IlmaValue ilma_finance_compound(IlmaValue principal, IlmaValue rate, IlmaValue years) {
    double p = to_num(principal);
    double r = to_num(rate);
    double y = to_num(years);
    double result = p * pow(1.0 + r, y);
    result = round(result * 100.0) / 100.0;
    return ilma_decimal(result);
}

IlmaValue ilma_finance_zakat(IlmaValue wealth, IlmaValue nisab) {
    double w = to_num(wealth);
    double n = to_num(nisab);
    if (w >= n) {
        double zakat = w * 0.025;
        zakat = round(zakat * 100.0) / 100.0;
        return ilma_decimal(zakat);
    }
    return ilma_decimal(0.0);
}

IlmaValue ilma_finance_profit(IlmaValue cost, IlmaValue revenue) {
    double c = to_num(cost);
    double r = to_num(revenue);
    return ilma_decimal(r - c);
}

IlmaValue ilma_finance_margin(IlmaValue cost, IlmaValue revenue) {
    double c = to_num(cost);
    double r = to_num(revenue);
    if (r == 0.0) return ilma_decimal(0.0);
    double margin = ((r - c) / r) * 100.0;
    return ilma_decimal(margin);
}

IlmaValue ilma_finance_simple_interest(IlmaValue principal, IlmaValue rate, IlmaValue years) {
    double p = to_num(principal);
    double r = to_num(rate);
    double y = to_num(years);
    return ilma_decimal(p * r * y);
}
