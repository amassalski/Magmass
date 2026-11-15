#include "ProgramyZewnetrzne.h"

#include "../Narzedzia/PolecenieKsiazek.h"

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/stream.h>
#include <wx/utils.h>

namespace ProgramyZewnetrzne {

namespace {

class Proces : public wxProcess {
public:
    Proces() : wxProcess(wxPROCESS_REDIRECT) {}
    bool koniec = false;
    int kod = -1;
    void OnTerminate(int, int status) override { koniec = true; kod = status; }
};

void oproznij(wxInputStream* s, std::string& cel) {
    while (s && s->CanRead()) {
        char bufor[1024];
        s->Read(bufor, sizeof bufor);
        const size_t n = s->LastRead();
        if (n == 0) break;
        if (cel.size() < 8192) cel.append(bufor, n);
    }
}

wxString szukajWPath(const wxString& nazwa) {
    wxPathList lista;
    lista.AddEnvList("PATH");
    for (const wxString& n : {nazwa, nazwa + ".exe", nazwa + ".bat", nazwa + ".cmd"}) {
        wxString p = lista.FindAbsoluteValidPath(n);
        if (!p.empty()) return p;
    }
    return {};
}

} // namespace

void DopiszTypoweKatalogiDoPATH() {
#ifndef __WXMSW__
    wxString stary;
    wxGetEnv("PATH", &stary);
    const std::string nowy = PolecenieKsiazek::rozszerzPATH(stary.utf8_string(), {"/opt/homebrew/bin", "/opt/homebrew/sbin", "/usr/local/bin", "/opt/local/bin"});
    if (nowy != stary.utf8_string()) wxSetEnv("PATH", wxString::FromUTF8(nowy));
#endif
}

wxString Znajdz(const std::string& nazwa, const std::string& zUstawien) {
    DopiszTypoweKatalogiDoPATH();
    wxLogNull cisza;
    const wxString ust = wxString::FromUTF8(zUstawien);
    if (!ust.empty()) {
        if (wxFileName::FileExists(ust)) return ust;
        if (ust.find('/') == wxString::npos && ust.find('\\') == wxString::npos) {   // sama nazwa: szukamy w PATH
            const wxString p = szukajWPath(ust);
            if (!p.empty()) return p;
        }
        return {};   // wskazano program, którego nie ma: nie podstawiamy innego po cichu
    }
    return szukajWPath(wxString::FromUTF8(nazwa));
}

Wynik Uruchom(const wxString& program, const std::vector<std::string>& argumenty, const std::function<bool()>& postep) {
    DopiszTypoweKatalogiDoPATH();
    Wynik w;
    std::vector<wxString> napisy;
    napisy.push_back(program);
    for (const std::string& a : argumenty) napisy.push_back(wxString::FromUTF8(a));
    std::vector<const wxChar*> argv;
    for (const wxString& n : napisy) argv.push_back(n.c_str());
    argv.push_back(nullptr);

    Proces* proces = new Proces;
    wxLogNull cisza;
    const long pid = wxExecute(argv.data(), wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE | wxEXEC_NODISABLE | wxEXEC_MAKE_GROUP_LEADER, proces);
    if (pid == 0) { delete proces; return w; }
    w.uruchomiono = true;
    wxLongLong przerwanoO = 0;
    while (!proces->koniec) {
        oproznij(proces->GetInputStream(), w.wyjscie);
        oproznij(proces->GetErrorStream(), w.wyjscie);
        if (postep && !postep() && !w.przerwano) {
            w.przerwano = true;
            przerwanoO = wxGetLocalTimeMillis();
            wxProcess::Kill(static_cast<int>(pid), wxSIGTERM, wxKILL_CHILDREN);
        }
        if (w.przerwano && wxGetLocalTimeMillis() - przerwanoO > 5000) {   // proces nie reaguje: odpinamy go i wychodzimy
            proces->Detach();
            w.kod = -1;
            return w;
        }
        wxMilliSleep(40);
        wxYield();
    }
    oproznij(proces->GetInputStream(), w.wyjscie);
    oproznij(proces->GetErrorStream(), w.wyjscie);
    w.kod = proces->kod;
    delete proces;   // OnTerminate nadpisane bez wywołania bazowego, więc obiekt nie usuwa się sam
    return w;
}

} // namespace ProgramyZewnetrzne
