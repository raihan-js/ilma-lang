#include "draw.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── Helper: extract a number from ILMA_WHOLE or ILMA_DECIMAL ── */

static double to_num(IlmaValue v) {
    if (v.type == ILMA_WHOLE)   return (double)v.as_whole;
    if (v.type == ILMA_DECIMAL) return v.as_decimal;
    return 0.0;
}

/* ── ilma_draw_canvas ─────────────────────────────────────────── */

IlmaValue ilma_draw_canvas(IlmaValue width, IlmaValue height) {
    IlmaValue book_val = ilma_notebook_new();
    IlmaBook* book = book_val.as_notebook;

    ilma_notebook_set(book, "width", width);
    ilma_notebook_set(book, "height", height);

    IlmaValue elements = ilma_bag_new();
    ilma_notebook_set(book, "elements", elements);

    ilma_notebook_set(book, "filename", ilma_text("drawing.svg"));

    return book_val;
}

/* ── ilma_draw_circle ─────────────────────────────────────────── */

IlmaValue ilma_draw_circle(IlmaValue canvas, IlmaValue cx, IlmaValue cy, IlmaValue r, IlmaValue color) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\"/>\n",
        to_num(cx), to_num(cy), to_num(r),
        color.type == ILMA_TEXT ? color.as_text : "black");

    IlmaValue elem = ilma_text(buf);
    IlmaValue elements = ilma_notebook_get(canvas.as_notebook, "elements");
    ilma_bag_add(elements.as_bag, elem);

    return canvas;
}

/* ── ilma_draw_rect ───────────────────────────────────────────── */

IlmaValue ilma_draw_rect(IlmaValue canvas, IlmaValue x, IlmaValue y, IlmaValue w, IlmaValue h, IlmaValue color) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" fill=\"%s\"/>\n",
        to_num(x), to_num(y), to_num(w), to_num(h),
        color.type == ILMA_TEXT ? color.as_text : "black");

    IlmaValue elem = ilma_text(buf);
    IlmaValue elements = ilma_notebook_get(canvas.as_notebook, "elements");
    ilma_bag_add(elements.as_bag, elem);

    return canvas;
}

/* ── ilma_draw_line ───────────────────────────────────────────── */

IlmaValue ilma_draw_line(IlmaValue canvas, IlmaValue x1, IlmaValue y1, IlmaValue x2, IlmaValue y2, IlmaValue color) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"%s\" stroke-width=\"2\"/>\n",
        to_num(x1), to_num(y1), to_num(x2), to_num(y2),
        color.type == ILMA_TEXT ? color.as_text : "black");

    IlmaValue elem = ilma_text(buf);
    IlmaValue elements = ilma_notebook_get(canvas.as_notebook, "elements");
    ilma_bag_add(elements.as_bag, elem);

    return canvas;
}

/* ── ilma_draw_text_elem ──────────────────────────────────────── */

IlmaValue ilma_draw_text_elem(IlmaValue canvas, IlmaValue x, IlmaValue y, IlmaValue text_val, IlmaValue color) {
    const char* txt = (text_val.type == ILMA_TEXT) ? text_val.as_text : "text";
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "  <text x=\"%.2f\" y=\"%.2f\" fill=\"%s\" font-family=\"sans-serif\" font-size=\"16\">%s</text>\n",
        to_num(x), to_num(y),
        color.type == ILMA_TEXT ? color.as_text : "black",
        txt);

    IlmaValue elem = ilma_text(buf);
    IlmaValue elements = ilma_notebook_get(canvas.as_notebook, "elements");
    ilma_bag_add(elements.as_bag, elem);

    return canvas;
}

/* ── ilma_draw_polygon ────────────────────────────────────────── */

IlmaValue ilma_draw_polygon(IlmaValue canvas, IlmaValue points_bag, IlmaValue color) {
    if (points_bag.type != ILMA_BAG || points_bag.as_bag == NULL) {
        return canvas;
    }

    IlmaBag* pts = points_bag.as_bag;
    int n = ilma_bag_size(pts);
    if (n == 0) return canvas;

    /* Build the points string: "x1,y1 x2,y2 ..." */
    char points_str[4096];
    points_str[0] = '\0';
    int offset = 0;

    for (int i = 0; i < n; i++) {
        IlmaValue point = ilma_bag_get(pts, i);
        if (point.type != ILMA_BAG || point.as_bag == NULL) continue;

        IlmaValue px = ilma_bag_get(point.as_bag, 0);
        IlmaValue py = ilma_bag_get(point.as_bag, 1);

        int written = snprintf(points_str + offset, sizeof(points_str) - offset,
            "%s%.2f,%.2f", (offset > 0 ? " " : ""), to_num(px), to_num(py));
        offset += written;
        if (offset >= (int)sizeof(points_str) - 1) break;
    }

    char buf[4608];
    snprintf(buf, sizeof(buf),
        "  <polygon points=\"%s\" fill=\"%s\"/>\n",
        points_str,
        color.type == ILMA_TEXT ? color.as_text : "black");

    IlmaValue elem = ilma_text(buf);
    IlmaValue elements = ilma_notebook_get(canvas.as_notebook, "elements");
    ilma_bag_add(elements.as_bag, elem);

    return canvas;
}

/* ── ilma_draw_save ───────────────────────────────────────────── */

IlmaValue ilma_draw_save(IlmaValue canvas) {
    IlmaBook* book = canvas.as_notebook;

    IlmaValue w_val = ilma_notebook_get(book, "width");
    IlmaValue h_val = ilma_notebook_get(book, "height");
    IlmaValue fn_val = ilma_notebook_get(book, "filename");
    IlmaValue elems_val = ilma_notebook_get(book, "elements");

    double w = to_num(w_val);
    double h = to_num(h_val);
    const char* filename = (fn_val.type == ILMA_TEXT) ? fn_val.as_text : "drawing.svg";

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return ilma_text("Error: could not open file for writing");
    }

    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %.0f %.0f\" width=\"%.0f\" height=\"%.0f\">\n",
        w, h, w, h);

    if (elems_val.type == ILMA_BAG && elems_val.as_bag != NULL) {
        IlmaBag* bag = elems_val.as_bag;
        int count = ilma_bag_size(bag);
        for (int i = 0; i < count; i++) {
            IlmaValue elem = ilma_bag_get(bag, i);
            if (elem.type == ILMA_TEXT && elem.as_text != NULL) {
                fprintf(fp, "%s", elem.as_text);
            }
        }
    }

    fprintf(fp, "</svg>\n");
    fclose(fp);

    char result[512];
    snprintf(result, sizeof(result), "Saved to %s", filename);
    return ilma_text(result);
}

/* ── ilma_draw_islamic_star ───────────────────────────────────── */

IlmaValue ilma_draw_islamic_star(IlmaValue canvas, IlmaValue cx, IlmaValue cy, IlmaValue size, IlmaValue points, IlmaValue color) {
    double center_x = to_num(cx);
    double center_y = to_num(cy);
    double outer_radius = to_num(size);
    double inner_radius = outer_radius * 0.4;
    int n = (int)to_num(points);

    if (n < 2) n = 2;

    /* Build the points string alternating outer and inner vertices */
    char points_str[8192];
    points_str[0] = '\0';
    int offset = 0;

    for (int i = 0; i < n; i++) {
        /* Outer vertex */
        double angle_outer = 2.0 * M_PI * i / n - M_PI / 2.0;
        double ox = center_x + outer_radius * cos(angle_outer);
        double oy = center_y + outer_radius * sin(angle_outer);

        int written = snprintf(points_str + offset, sizeof(points_str) - offset,
            "%s%.2f,%.2f", (offset > 0 ? " " : ""), ox, oy);
        offset += written;
        if (offset >= (int)sizeof(points_str) - 1) break;

        /* Inner vertex */
        double angle_inner = 2.0 * M_PI * i / n + M_PI / n - M_PI / 2.0;
        double ix = center_x + inner_radius * cos(angle_inner);
        double iy = center_y + inner_radius * sin(angle_inner);

        written = snprintf(points_str + offset, sizeof(points_str) - offset,
            " %.2f,%.2f", ix, iy);
        offset += written;
        if (offset >= (int)sizeof(points_str) - 1) break;
    }

    char buf[8704];
    snprintf(buf, sizeof(buf),
        "  <polygon points=\"%s\" fill=\"%s\"/>\n",
        points_str,
        color.type == ILMA_TEXT ? color.as_text : "black");

    IlmaValue elem = ilma_text(buf);
    IlmaValue elements = ilma_notebook_get(canvas.as_notebook, "elements");
    ilma_bag_add(elements.as_bag, elem);

    return canvas;
}
