#include "think.h"
#include <time.h>

IlmaValue ilma_think_stoic_question(void) {
    static const char* questions[12] = {
        "What is within your control today?",
        "What would the wisest version of you do right now?",
        "Is this problem permanent, or will it pass?",
        "What would you advise a friend in this situation?",
        "What one thing, if done today, would matter most?",
        "Am I acting from fear or from values?",
        "What can I learn from this difficulty?",
        "Is this thought true, useful, and kind?",
        "What would I regret not doing in ten years?",
        "Where am I wasting energy on things I cannot change?",
        "What does gratitude look like right now?",
        "How can I serve someone else today?"
    };

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    int index = t->tm_yday % 12;
    return ilma_text(questions[index]);
}

IlmaValue ilma_think_pros_cons_new(IlmaValue question) {
    IlmaValue book_val = ilma_notebook_new();
    IlmaBook* book = book_val.as_notebook;
    ilma_notebook_set(book, "question", question);

    IlmaValue pros_val = ilma_bag_new();
    ilma_notebook_set(book, "pros", pros_val);

    IlmaValue cons_val = ilma_bag_new();
    ilma_notebook_set(book, "cons", cons_val);

    return book_val;
}

IlmaValue ilma_think_weigh_result(IlmaValue pros_bag, IlmaValue cons_bag) {
    int pros_count = 0;
    int cons_count = 0;

    if (pros_bag.type == ILMA_BAG && pros_bag.as_bag != NULL) {
        pros_count = (int)pros_bag.as_bag->count;
    }
    if (cons_bag.type == ILMA_BAG && cons_bag.as_bag != NULL) {
        cons_count = (int)cons_bag.as_bag->count;
    }

    if (pros_count > cons_count) return ilma_text("pros");
    if (cons_count > pros_count) return ilma_text("cons");
    return ilma_text("balanced");
}
