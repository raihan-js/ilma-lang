#ifndef ILMA_SCIENCE_H
#define ILMA_SCIENCE_H

#include "ilma_runtime.h"

IlmaValue ilma_science_gravity(IlmaValue mass_kg);
IlmaValue ilma_science_kinetic_energy(IlmaValue mass_kg, IlmaValue velocity_ms);
IlmaValue ilma_science_potential_energy(IlmaValue mass_kg, IlmaValue height_m);
IlmaValue ilma_science_speed(IlmaValue distance_m, IlmaValue time_s);
IlmaValue ilma_science_celsius_to_fahrenheit(IlmaValue celsius);
IlmaValue ilma_science_fahrenheit_to_celsius(IlmaValue fahrenheit);
IlmaValue ilma_science_celsius_to_kelvin(IlmaValue celsius);
IlmaValue ilma_science_ohms_law_voltage(IlmaValue current_a, IlmaValue resistance_ohm);
IlmaValue ilma_science_ohms_law_current(IlmaValue voltage_v, IlmaValue resistance_ohm);
IlmaValue ilma_science_light_years_to_km(IlmaValue light_years);
IlmaValue ilma_science_atom_count(IlmaValue mass_g, IlmaValue molar_mass);

#endif /* ILMA_SCIENCE_H */
