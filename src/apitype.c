#include "apitype.h"

int apitype_detect_category(int apitype) {
    // VSH (XMB)
    if (apitype >= 0x200) {
        return CAT_VSH;
    }
    
    // POPS (PS1)
    if (apitype == 0x144 || apitype == 0x155) {
        return CAT_PS1;
    }
    
    // UMD / ISO (jogos PSP)
    if (apitype == 0x120 || apitype == 0x160 ||
       (apitype >= 0x123 && apitype <= 0x126) ||
        apitype == 0x130 ||
       (apitype >= 0x110 && apitype <= 0x115)) {
        return CAT_PSP;
    }
    
    // Homebrew
    if (apitype == 0x141 || apitype == 0x152) {
        return CAT_HOMEBREW;
    }
    
    // Some POPS variations might use something else like 0x143 
    // Fallbacks can be placed here if needed.
    if (apitype == 0x143) {
        return CAT_PS1;
    }

    return CAT_UNKNOWN;
}
