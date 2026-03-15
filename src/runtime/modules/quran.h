#ifndef ILMA_QURAN_H
#define ILMA_QURAN_H

#include "ilma_runtime.h"

IlmaValue ilma_quran_ayah_of_the_day(void);
IlmaValue ilma_quran_search(IlmaValue keyword);
IlmaValue ilma_quran_surah(IlmaValue name);

#endif /* ILMA_QURAN_H */
