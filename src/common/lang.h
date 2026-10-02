// common/lang.h — language enum -> gamedata folder code
// Single source of truth shared by game, editor, and bot.

#ifndef COMMON_LANG_H
#define COMMON_LANG_H

inline const char* LangFolderForIndex(int idx)
{
    switch (idx) {
        case 0: return "EN";
        case 1: return "DE";
        case 2: return "FR";
        case 3: return "HU";
        case 4: return "CZ";
        case 5: return "SP";
        case 6: return "IT";
        case 7: return "RU";
        case 8: return "ZH";
        default: return "EN";
    }
}

#endif // COMMON_LANG_H
