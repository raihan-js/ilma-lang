#ifndef ILMA_FINANCE_H
#define ILMA_FINANCE_H

#include "ilma_runtime.h"

IlmaValue ilma_finance_compound(IlmaValue principal, IlmaValue rate, IlmaValue years);
IlmaValue ilma_finance_zakat(IlmaValue wealth, IlmaValue nisab);
IlmaValue ilma_finance_profit(IlmaValue cost, IlmaValue revenue);
IlmaValue ilma_finance_margin(IlmaValue cost, IlmaValue revenue);
IlmaValue ilma_finance_simple_interest(IlmaValue principal, IlmaValue rate, IlmaValue years);

#endif /* ILMA_FINANCE_H */
