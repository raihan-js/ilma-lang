#ifndef ILMA_TRADE_H
#define ILMA_TRADE_H

#include "ilma_runtime.h"

IlmaValue ilma_trade_profit(IlmaValue cost, IlmaValue revenue);
IlmaValue ilma_trade_margin(IlmaValue cost, IlmaValue revenue);
IlmaValue ilma_trade_markup(IlmaValue cost, IlmaValue selling_price);
IlmaValue ilma_trade_break_even(IlmaValue fixed_costs, IlmaValue price_per_unit, IlmaValue cost_per_unit);
IlmaValue ilma_trade_supply_demand_price(IlmaValue base_price, IlmaValue demand_change_pct, IlmaValue supply_change_pct);
IlmaValue ilma_trade_vat(IlmaValue price, IlmaValue vat_rate_pct);
IlmaValue ilma_trade_discount(IlmaValue price, IlmaValue discount_pct);
IlmaValue ilma_trade_halal_check(IlmaValue involves_interest, IlmaValue involves_gambling, IlmaValue involves_alcohol);

#endif /* ILMA_TRADE_H */
