#ifndef ILMA_NUMBER_H
#define ILMA_NUMBER_H

#include "ilma_runtime.h"

IlmaValue ilma_number_sqrt(IlmaValue n);
IlmaValue ilma_number_abs(IlmaValue n);
IlmaValue ilma_number_floor(IlmaValue n);
IlmaValue ilma_number_ceil(IlmaValue n);
IlmaValue ilma_number_round(IlmaValue n);
IlmaValue ilma_number_power(IlmaValue base, IlmaValue exp);
IlmaValue ilma_number_random(IlmaValue min, IlmaValue max);
IlmaValue ilma_number_is_prime(IlmaValue n);
IlmaValue ilma_number_fibonacci(IlmaValue n);
IlmaValue ilma_number_to_binary(IlmaValue n);
IlmaValue ilma_number_to_hex(IlmaValue n);
IlmaValue ilma_number_pi(void);
IlmaValue ilma_number_e(void);

#endif /* ILMA_NUMBER_H */
