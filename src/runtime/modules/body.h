#ifndef ILMA_BODY_H
#define ILMA_BODY_H

#include "ilma_runtime.h"

IlmaValue ilma_body_bmi(IlmaValue weight_kg, IlmaValue height_cm);
IlmaValue ilma_body_bmi_category(IlmaValue bmi);
IlmaValue ilma_body_daily_water(IlmaValue weight_kg);
IlmaValue ilma_body_sleep_advice(IlmaValue hours, IlmaValue age);

#endif /* ILMA_BODY_H */
