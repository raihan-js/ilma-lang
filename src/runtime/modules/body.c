#include "body.h"
#include <math.h>

static double to_num(IlmaValue v) {
    if (v.type == ILMA_WHOLE) return (double)v.as_whole;
    if (v.type == ILMA_DECIMAL) return v.as_decimal;
    return 0.0;
}

IlmaValue ilma_body_bmi(IlmaValue weight_kg, IlmaValue height_cm) {
    double weight = to_num(weight_kg);
    double height_m = to_num(height_cm) / 100.0;
    if (height_m <= 0.0) return ilma_decimal(0.0);
    double bmi = weight / (height_m * height_m);
    bmi = round(bmi * 10.0) / 10.0;
    return ilma_decimal(bmi);
}

IlmaValue ilma_body_bmi_category(IlmaValue bmi) {
    double b = to_num(bmi);
    if (b < 18.5) return ilma_text("underweight");
    if (b < 25.0) return ilma_text("healthy");
    if (b < 30.0) return ilma_text("overweight");
    return ilma_text("obese");
}

IlmaValue ilma_body_daily_water(IlmaValue weight_kg) {
    double weight = to_num(weight_kg);
    double litres = weight * 0.033;
    litres = round(litres * 10.0) / 10.0;
    return ilma_decimal(litres);
}

IlmaValue ilma_body_sleep_advice(IlmaValue hours, IlmaValue age) {
    double h = to_num(hours);
    double a = to_num(age);

    double ideal_low, ideal_high;

    if (a < 12.0) {
        ideal_low = 9.0;
        ideal_high = 11.0;
    } else if (a < 18.0) {
        ideal_low = 8.0;
        ideal_high = 10.0;
    } else {
        ideal_low = 7.0;
        ideal_high = 9.0;
    }

    if (h < ideal_low) {
        return ilma_text("You may need more sleep");
    } else if (h > ideal_high) {
        return ilma_text("That may be too much sleep");
    } else {
        return ilma_text("Just right for your age");
    }
}
