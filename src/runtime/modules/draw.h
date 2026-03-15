#ifndef ILMA_DRAW_H
#define ILMA_DRAW_H

#include "ilma_runtime.h"

IlmaValue ilma_draw_canvas(IlmaValue width, IlmaValue height);
IlmaValue ilma_draw_circle(IlmaValue canvas, IlmaValue cx, IlmaValue cy, IlmaValue r, IlmaValue color);
IlmaValue ilma_draw_rect(IlmaValue canvas, IlmaValue x, IlmaValue y, IlmaValue w, IlmaValue h, IlmaValue color);
IlmaValue ilma_draw_line(IlmaValue canvas, IlmaValue x1, IlmaValue y1, IlmaValue x2, IlmaValue y2, IlmaValue color);
IlmaValue ilma_draw_text_elem(IlmaValue canvas, IlmaValue x, IlmaValue y, IlmaValue text_val, IlmaValue color);
IlmaValue ilma_draw_polygon(IlmaValue canvas, IlmaValue points_bag, IlmaValue color);
IlmaValue ilma_draw_save(IlmaValue canvas);
IlmaValue ilma_draw_islamic_star(IlmaValue canvas, IlmaValue cx, IlmaValue cy, IlmaValue size, IlmaValue points, IlmaValue color);

#endif /* ILMA_DRAW_H */
