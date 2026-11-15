#ifndef KLUCZOBUDOWY_H
#define KLUCZOBUDOWY_H

#include <cctype>
#include <string>

/// Klucz porównania nazw obudów: małe litery i cyfry ASCII, bez reszty znaków ("TO-220" == "to220", "R7.5" == "r75")
inline std::string kluczObudowy(const std::string& nazwa) {
    std::string k;
    for (unsigned char c : nazwa)
        if (c < 0x80 && std::isalnum(c)) k += static_cast<char>(std::tolower(c));
    return k;
}

#endif // KLUCZOBUDOWY_H
