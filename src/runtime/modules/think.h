#ifndef ILMA_THINK_H
#define ILMA_THINK_H

#include "ilma_runtime.h"

IlmaValue ilma_think_stoic_question(void);
IlmaValue ilma_think_pros_cons_new(IlmaValue question);
IlmaValue ilma_think_weigh_result(IlmaValue pros_bag, IlmaValue cons_bag);

#endif /* ILMA_THINK_H */
