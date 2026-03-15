#include "quran.h"
#include <time.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char* surah;
    int number;
    const char* arabic;
    const char* translation;
} IlmaQuranAyah;

static const IlmaQuranAyah ilma_quran_ayat[30] = {
    {"Al-Alaq", 1, "Iqra bismi rabbika alladhi khalaq", "Read in the name of your Lord who created"},
    {"Al-Baqarah", 177, "", "Righteousness is not that you turn your faces toward the east or the west, but true righteousness is in one who believes in Allah"},
    {"Al-Baqarah", 286, "", "Allah does not burden a soul beyond that it can bear"},
    {"Al-Imran", 190, "", "In the creation of the heavens and earth are signs for those of understanding"},
    {"Al-Imran", 200, "", "Be patient, persevere, remain steadfast"},
    {"An-Nisa", 36, "", "Worship Allah and associate nothing with Him, and to parents do good"},
    {"Al-Maidah", 2, "", "Cooperate in righteousness and piety"},
    {"Al-An'am", 82, "", "Those who believe and do not mix their belief with injustice"},
    {"Al-Anfal", 2, "", "True believers are those whose hearts tremble at the mention of Allah"},
    {"Al-Tawbah", 119, "", "Be with the truthful"},
    {"Yunus", 62, "", "Unquestionably, the allies of Allah — no fear will there be concerning them"},
    {"Yusuf", 87, "", "Do not despair of relief from Allah"},
    {"Al-Ra'd", 11, "", "Allah will not change the condition of a people until they change what is in themselves"},
    {"Al-Isra", 36, "", "Do not pursue that of which you have no knowledge"},
    {"Al-Kahf", 46, "", "Wealth and children are the adornment of the worldly life"},
    {"Ta-Ha", 114, "", "My Lord, increase me in knowledge"},
    {"Al-Anbiya", 107, "", "We have not sent you except as a mercy to the worlds"},
    {"Al-Mu'minun", 1, "", "Successful indeed are the believers"},
    {"Al-Furqan", 63, "", "The servants of the Most Merciful walk upon the earth humbly"},
    {"Al-Qasas", 77, "", "Seek the home of the Hereafter, but do not forget your share of this world"},
    {"Al-Ankabut", 69, "", "Those who strive for Us, We will surely guide them to Our ways"},
    {"Al-Rum", 21, "", "Among His signs is that He created for you mates from among yourselves"},
    {"Luqman", 17, "", "Establish prayer, enjoin what is right, forbid what is wrong, and be patient"},
    {"Az-Zumar", 9, "", "Are those who know equal to those who do not know?"},
    {"Ghafir", 60, "", "Your Lord says: Call upon Me; I will respond to you"},
    {"Ash-Shura", 38, "", "Those who conduct their affairs by mutual consultation"},
    {"Al-Hujurat", 13, "", "The most noble of you in the sight of Allah is the most righteous of you"},
    {"Al-Hadid", 4, "", "He is with you wherever you are"},
    {"Al-Mujadila", 11, "", "Allah will raise those who have believed among you and those who were given knowledge"},
    {"Al-Asr", 1, "", "By time, indeed mankind is in loss, except those who believe and do righteous deeds"},
};

static IlmaValue ayah_to_notebook(const IlmaQuranAyah* a) {
    IlmaValue book_val = ilma_notebook_new();
    IlmaBook* book = book_val.as_notebook;
    ilma_notebook_set(book, "surah", ilma_text(a->surah));
    ilma_notebook_set(book, "number", ilma_whole((int64_t)a->number));
    ilma_notebook_set(book, "arabic", ilma_text(a->arabic));
    ilma_notebook_set(book, "translation", ilma_text(a->translation));
    return book_val;
}

/* Case-insensitive substring search (portable replacement for strcasestr) */
static const char* ilma_strcasestr(const char* haystack, const char* needle) {
    if (needle[0] == '\0') return haystack;
    size_t nlen = strlen(needle);
    for (; *haystack != '\0'; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            size_t i;
            for (i = 1; i < nlen; i++) {
                if (haystack[i] == '\0') return NULL;
                if (tolower((unsigned char)haystack[i]) != tolower((unsigned char)needle[i])) break;
            }
            if (i == nlen) return haystack;
        }
    }
    return NULL;
}

IlmaValue ilma_quran_ayah_of_the_day(void) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    int index = t->tm_yday % 30;
    return ayah_to_notebook(&ilma_quran_ayat[index]);
}

IlmaValue ilma_quran_search(IlmaValue keyword) {
    const char* kw = "";
    if (keyword.type == ILMA_TEXT && keyword.as_text != NULL) {
        kw = keyword.as_text;
    }

    IlmaValue bag_val = ilma_bag_new();
    IlmaBag* bag = bag_val.as_bag;

    for (int i = 0; i < 30; i++) {
        if (ilma_strcasestr(ilma_quran_ayat[i].translation, kw) != NULL) {
            IlmaValue entry = ayah_to_notebook(&ilma_quran_ayat[i]);
            ilma_bag_add(bag, entry);
        }
    }

    return bag_val;
}

IlmaValue ilma_quran_surah(IlmaValue name) {
    const char* sname = "";
    if (name.type == ILMA_TEXT && name.as_text != NULL) {
        sname = name.as_text;
    }

    for (int i = 0; i < 30; i++) {
        if (ilma_strcasestr(ilma_quran_ayat[i].surah, sname) != NULL) {
            return ayah_to_notebook(&ilma_quran_ayat[i]);
        }
    }

    return ilma_empty_val();
}
