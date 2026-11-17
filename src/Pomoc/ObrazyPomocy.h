#ifndef OBRAZYPOMOCY_H
#define OBRAZYPOMOCY_H

#include <cstddef>
#include <vector>

/// Ilustracje do pomocy wbudowane w program (PNG), wygenerowane skryptem narzedzia/zbuduj_obrazy_pomocy.py.
/// W pomocy występują jako <img src="obrazy/nazwa.png">.
struct ObrazPomocy {
    const char* nazwa;
    const unsigned char* dane;
    size_t rozmiar;
};
const std::vector<ObrazPomocy>& ObrazyPomocy();

#endif // OBRAZYPOMOCY_H
