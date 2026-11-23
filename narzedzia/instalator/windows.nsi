; Instalator Magmass (NSIS 3, Unicode). Parametry przekazuje zbuduj_windows.sh przez makensis -D:
;   WERSJA, STAGE (katalog z gotowym programem), WYNIK (plik wyjściowy), IKONA (opcjonalnie, .ico), LICENCJA (opcjonalnie, tekst licencji do zatwierdzenia)
Unicode true
!include "MUI2.nsh"

Name "Magmass"
OutFile "${WYNIK}"
InstallDir "$PROGRAMFILES64\Magmass"
InstallDirRegKey HKLM "Software\Magmass" "Katalog"
RequestExecutionLevel admin
; Skróty menu Start i pulpitu: SetShellVarContext all w każdej sekcji (instalator z podniesionymi uprawnieniami tworzyłby je
; inaczej w profilu konta administratora, które może być innym kontem niż użytkownika, więc skrótów nie byłoby widać).
; Plik MUSI być zapisany jako UTF-8 z BOM, inaczej makensis czyta go jako ANSI i polskie litery są zepsute.
SetCompressor /SOLID lzma

VIProductVersion "${WERSJA}.0"
VIAddVersionKey /LANG=1045 "ProductName" "Magmass"
VIAddVersionKey /LANG=1045 "FileDescription" "Instalator programu Magmass"
VIAddVersionKey /LANG=1045 "FileVersion" "${WERSJA}"
VIAddVersionKey /LANG=1045 "LegalCopyright" "© 2026 Adam Massalski, GNU GPL 3 lub nowsza"

!ifdef IKONA
  !define MUI_ICON "${IKONA}"
  !define MUI_UNICON "${IKONA}"
!endif
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\Magmass.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Uruchom Magmass"

!insertmacro MUI_PAGE_WELCOME
!ifdef LICENCJA
  !define MUI_LICENSEPAGE_CHECKBOX
  !insertmacro MUI_PAGE_LICENSE "${LICENCJA}"
!endif
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_COMPONENTSPAGE_TEXT_TOP "Zaznacz skróty, które instalator ma utworzyć. Program zostanie zainstalowany zawsze."
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Polish"

Section "Program (wymagany)" SecProgram
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File /r "${STAGE}\*.*"
  WriteUninstaller "$INSTDIR\Odinstaluj.exe"


  WriteRegStr HKLM "Software\Magmass" "Katalog" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magmass" "DisplayName" "Magmass"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magmass" "DisplayVersion" "${WERSJA}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magmass" "UninstallString" '"$INSTDIR\Odinstaluj.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magmass" "DisplayIcon" "$INSTDIR\Magmass.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magmass" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magmass" "NoRepair" 1
SectionEnd

Section "Skrót w menu Start" SecStart
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\Magmass"
  CreateShortcut "$SMPROGRAMS\Magmass\Magmass.lnk" "$INSTDIR\Magmass.exe" "" "$INSTDIR\Magmass.exe" 0
  CreateShortcut "$SMPROGRAMS\Magmass\Odinstaluj.lnk" "$INSTDIR\Odinstaluj.exe"
SectionEnd

Section "Skrót na pulpicie" SecPulpit
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  CreateShortcut "$DESKTOP\Magmass.lnk" "$INSTDIR\Magmass.exe" "" "$INSTDIR\Magmass.exe" 0
SectionEnd

; Domyślnie wyłączony (/o): klasyczny pasek szybkiego uruchamiania istnieje tylko, jeśli użytkownik włączył go w systemie.
Section /o "Skrót na pasku szybkiego uruchamiania" SecSzybki
  SetShellVarContext current   ; Quick Launch jest zawsze folderem bieżącego użytkownika
  SetOutPath "$INSTDIR"
  CreateDirectory "$QUICKLAUNCH"
  CreateShortcut "$QUICKLAUNCH\Magmass.lnk" "$INSTDIR\Magmass.exe" "" "$INSTDIR\Magmass.exe" 0
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecProgram} "Program Magmass wraz z bibliotekami. Wymagany."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStart} "Dodaje Magmass i skrót do odinstalowania do menu Start."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecPulpit} "Tworzy skrót do programu na pulpicie."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSzybki} "Tworzy skrót w folderze paska szybkiego uruchamiania (Quick Launch). Widoczny, jeśli ten pasek jest włączony w systemie; przypięcie do paska zadań Windows można zrobić ręcznie (prawy klik na skrócie)."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
  SetShellVarContext all
  ; Dane użytkownika (konfiguracja w %APPDATA%\DomowyMagazyn) zostają, baza danych też.
  Delete "$DESKTOP\Magmass.lnk"
  SetShellVarContext current
  Delete "$QUICKLAUNCH\Magmass.lnk"
  Delete "$DESKTOP\Magmass.lnk"   ; skrót z wcześniejszych wersji instalatora (profil użytkownika)
  RMDir /r "$SMPROGRAMS\Magmass"
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\Magmass"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magmass"
  DeleteRegKey HKLM "Software\Magmass"
SectionEnd
