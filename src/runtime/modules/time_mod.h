#ifndef ILMA_TIME_MOD_H
#define ILMA_TIME_MOD_H

#include "ilma_runtime.h"

IlmaValue ilma_time_today_str(void);
IlmaValue ilma_time_to_hijri(IlmaValue gregorian_str);
IlmaValue ilma_time_days_between(IlmaValue date1_str, IlmaValue date2_str);

#endif /* ILMA_TIME_MOD_H */
