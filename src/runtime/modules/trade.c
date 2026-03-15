#include "trade.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double to_num(IlmaValue v) {
    if (v.type == ILMA_WHOLE) return (double)v.as_whole;
    if (v.type == ILMA_DECIMAL) return v.as_decimal;
    return 0.0;
}

IlmaValue ilma_trade_profit(IlmaValue cost, IlmaValue revenue) {
    double c = to_num(cost);
    double r = to_num(revenue);
    return ilma_decimal(r - c);
}

IlmaValue ilma_trade_margin(IlmaValue cost, IlmaValue revenue) {
    double c = to_num(cost);
    double r = to_num(revenue);
    if (r == 0.0) return ilma_decimal(0.0);
    double margin = ((r - c) / r) * 100.0;
    /* Apply %g-style rounding to avoid floating-point noise */
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", margin);
    return ilma_decimal(atof(buf));
}

IlmaValue ilma_trade_markup(IlmaValue cost, IlmaValue selling_price) {
    double c = to_num(cost);
    double sp = to_num(selling_price);
    if (c == 0.0) return ilma_decimal(0.0);
    return ilma_decimal(((sp - c) / c) * 100.0);
}

IlmaValue ilma_trade_break_even(IlmaValue fixed_costs, IlmaValue price_per_unit, IlmaValue cost_per_unit) {
    double fc = to_num(fixed_costs);
    double ppu = to_num(price_per_unit);
    double cpu = to_num(cost_per_unit);
    double denom = ppu - cpu;
    if (denom == 0.0) return ilma_decimal(0.0);
    return ilma_decimal(fc / denom);
}

IlmaValue ilma_trade_supply_demand_price(IlmaValue base_price, IlmaValue demand_change_pct, IlmaValue supply_change_pct) {
    double bp = to_num(base_price);
    double dc = to_num(demand_change_pct);
    double sc = to_num(supply_change_pct);
    double denom = 1.0 + sc / 100.0;
    if (denom == 0.0) return ilma_decimal(0.0);
    return ilma_decimal(bp * (1.0 + dc / 100.0) / denom);
}

IlmaValue ilma_trade_vat(IlmaValue price, IlmaValue vat_rate_pct) {
    double p = to_num(price);
    double vat = to_num(vat_rate_pct);
    return ilma_decimal(p * (1.0 + vat / 100.0));
}

IlmaValue ilma_trade_discount(IlmaValue price, IlmaValue discount_pct) {
    double p = to_num(price);
    double d = to_num(discount_pct);
    return ilma_decimal(p * (1.0 - d / 100.0));
}

IlmaValue ilma_trade_halal_check(IlmaValue involves_interest, IlmaValue involves_gambling, IlmaValue involves_alcohol) {
    IlmaValue result = ilma_notebook_new();

    if (ilma_is_truthy(involves_interest)) {
        ilma_notebook_set(result.as_notebook, "permissible", ilma_text("no"));
        ilma_notebook_set(result.as_notebook, "reason", ilma_text("Interest (riba) is not permitted"));
        return result;
    }

    if (ilma_is_truthy(involves_gambling)) {
        ilma_notebook_set(result.as_notebook, "permissible", ilma_text("no"));
        ilma_notebook_set(result.as_notebook, "reason", ilma_text("Gambling (maysir) is not permitted"));
        return result;
    }

    if (ilma_is_truthy(involves_alcohol)) {
        ilma_notebook_set(result.as_notebook, "permissible", ilma_text("no"));
        ilma_notebook_set(result.as_notebook, "reason", ilma_text("Alcohol trade is not permitted"));
        return result;
    }

    ilma_notebook_set(result.as_notebook, "permissible", ilma_text("yes"));
    ilma_notebook_set(result.as_notebook, "reason", ilma_text("Trade meets halal criteria"));
    return result;
}
