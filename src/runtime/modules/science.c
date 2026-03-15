#include "science.h"
#include <math.h>

static double to_num(IlmaValue v) {
    if (v.type == ILMA_WHOLE) return (double)v.as_whole;
    if (v.type == ILMA_DECIMAL) return v.as_decimal;
    return 0.0;
}

IlmaValue ilma_science_gravity(IlmaValue mass_kg) {
    double mass = to_num(mass_kg);
    return ilma_decimal(mass * 9.81);
}

IlmaValue ilma_science_kinetic_energy(IlmaValue mass_kg, IlmaValue velocity_ms) {
    double mass = to_num(mass_kg);
    double velocity = to_num(velocity_ms);
    return ilma_decimal(0.5 * mass * velocity * velocity);
}

IlmaValue ilma_science_potential_energy(IlmaValue mass_kg, IlmaValue height_m) {
    double mass = to_num(mass_kg);
    double height = to_num(height_m);
    return ilma_decimal(mass * 9.81 * height);
}

IlmaValue ilma_science_speed(IlmaValue distance_m, IlmaValue time_s) {
    double distance = to_num(distance_m);
    double time = to_num(time_s);
    if (time == 0.0) return ilma_decimal(0.0);
    return ilma_decimal(distance / time);
}

IlmaValue ilma_science_celsius_to_fahrenheit(IlmaValue celsius) {
    double c = to_num(celsius);
    return ilma_decimal((c * 9.0 / 5.0) + 32.0);
}

IlmaValue ilma_science_fahrenheit_to_celsius(IlmaValue fahrenheit) {
    double f = to_num(fahrenheit);
    return ilma_decimal((f - 32.0) * 5.0 / 9.0);
}

IlmaValue ilma_science_celsius_to_kelvin(IlmaValue celsius) {
    double c = to_num(celsius);
    return ilma_decimal(c + 273.15);
}

IlmaValue ilma_science_ohms_law_voltage(IlmaValue current_a, IlmaValue resistance_ohm) {
    double i = to_num(current_a);
    double r = to_num(resistance_ohm);
    return ilma_decimal(i * r);
}

IlmaValue ilma_science_ohms_law_current(IlmaValue voltage_v, IlmaValue resistance_ohm) {
    double v = to_num(voltage_v);
    double r = to_num(resistance_ohm);
    if (r == 0.0) return ilma_decimal(0.0);
    return ilma_decimal(v / r);
}

IlmaValue ilma_science_light_years_to_km(IlmaValue light_years) {
    double ly = to_num(light_years);
    return ilma_decimal(ly * 9.461e12);
}

IlmaValue ilma_science_atom_count(IlmaValue mass_g, IlmaValue molar_mass) {
    double mass = to_num(mass_g);
    double mm = to_num(molar_mass);
    if (mm == 0.0) return ilma_decimal(0.0);
    return ilma_decimal((mass / mm) * 6.022e23);
}
