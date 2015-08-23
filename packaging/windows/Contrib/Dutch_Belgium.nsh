;Language:Dutch_Belgium (2067)
;By Ronny Desmedt

!insertmacro LANGFILE "Dutch_Belgium" "Dutch_Belgium"

!ifdef MUI_WELCOMEPAGE
  ${LangFileString} MUI_TEXT_WELCOME_INFO_TITLE "Welkom bij $(^NameDA) installatie Wizard"
  ${LangFileString} MUI_TEXT_WELCOME_INFO_TEXT "Deze wizard zal u begeleiden bij de installatie van $(^NameDA).$\r$\n$\r$\nHet is aanbevol dat u alle andere programmas afsluit voordat u deze installatie uitvoerd. Dit geeft de mogelijkheid om relevante systeem bestanden bij te werken zonder dat uw systeem terug moet opstarten.$\r$\n$\r$\n$_CLICK"
!endif

!ifdef MUI_UNWELCOMEPAGE
  ${LangFileString} MUI_UNTEXT_WELCOME_INFO_TITLE "Welkom bij de $(^NameDA) deïnstallatie wizard"
  ${LangFileString} MUI_UNTEXT_WELCOME_INFO_TEXT "Deze wizard zal u begeleiden bij de deïnstallatie van $(^NameDA).$\r$\n$\r$\nControleer of $(^NameDA) is afgesloten alvorens de deïnstallatie te starten.$\r$\n$\r$\n$_CLICK"
!endif

!ifdef MUI_LICENSEPAGE
  ${LangFileString} MUI_TEXT_LICENSE_TITLE "Licentie overeenkomst"
  ${LangFileString} MUI_TEXT_LICENSE_SUBTITLE "Gelieve de licentie te lezen alvorens U $(^NameDA) installeert."
  ${LangFileString} MUI_INNERTEXT_LICENSE_BOTTOM "Klik op akkoord om de overeenkomst te aanvaarden. U moet de overeenkomst aanvaarden om $(^NameDA) te installeren."
  ${LangFileString} MUI_INNERTEXT_LICENSE_BOTTOM_CHECKBOX "Als u de voorwaarden van de overeenkomst aanvaard, Klik op onderstaande selectievakje. U moet de overeenkomst aanvaarden om $(^NameDA) te installeren. $_CLICK"
  ${LangFileString} MUI_INNERTEXT_LICENSE_BOTTOM_RADIOBUTTONS "Als u de voorwaarden van de overeenkomst aanvaard, selecteer de eerste onderstaande optie. U moet de overeenkomst aanvaarden om $(^NameDA) te installeren. $_CLICK"
!endif

!ifdef MUI_UNLICENSEPAGE
  ${LangFileString} MUI_UNTEXT_LICENSE_TITLE "Licentie overeenkomst"
  ${LangFileString} MUI_UNTEXT_LICENSE_SUBTITLE "Gelieve de licentie overeenkomst te herlezen alvorens met de deïnstallatie van $(^NameDA) verder te doen."
  ${LangFileString} MUI_UNINNERTEXT_LICENSE_BOTTOM "Klik op akkoord om de overeenkomst te aanvaarden. U moet de overeenkomst aanvaarden om $(^NameDA) te deïnstalleren."
  ${LangFileString} MUI_UNINNERTEXT_LICENSE_BOTTOM_CHECKBOX "Als u de voorwaarden van de overeenkomst aanvaard, Klik op onderstaande selectievakje. U moet de overeenkomst aanvaarden om  $(^NameDA) te deïnstalleren. $_CLICK"
  ${LangFileString} MUI_UNINNERTEXT_LICENSE_BOTTOM_RADIOBUTTONS "Als u de voorwaarden van de overeenkomst aanvaard, selecteer de eerste onderstaande optie. U moet de overeenkomst aanvaarden om $(^NameDA) te deïnstalleren. $_CLICK"
!endif

!ifdef MUI_LICENSEPAGE | MUI_UNLICENSEPAGE
  ${LangFileString} MUI_INNERTEXT_LICENSE_TOP "Gebruik pagina neer om de rest van de overeenkomst te lezen."
!endif

!ifdef MUI_COMPONENTSPAGE
  ${LangFileString} MUI_TEXT_COMPONENTS_TITLE "Kies onderdelen"
  ${LangFileString} MUI_TEXT_COMPONENTS_SUBTITLE "Kies de onderdelen van $(^NameDA) die u wilt installeren."
!endif

!ifdef MUI_UNCOMPONENTSPAGE
  ${LangFileString} MUI_UNTEXT_COMPONENTS_TITLE "Kies onderdelen"
  ${LangFileString} MUI_UNTEXT_COMPONENTS_SUBTITLE "Kies de onderdelen van $(^NameDA) die u wilt deïnstalleren."
!endif

!ifdef MUI_COMPONENTSPAGE | MUI_UNCOMPONENTSPAGE
  ${LangFileString} MUI_INNERTEXT_COMPONENTS_DESCRIPTION_TITLE "Beschrijving"
  !ifndef NSIS_CONFIG_COMPONENTPAGE_ALTERNATIVE
    ${LangFileString} MUI_INNERTEXT_COMPONENTS_DESCRIPTION_INFO "Beweeg de muisaanwijzer over de onderdelen om de beschrijving te zien."
  !else
    ${LangFileString} MUI_INNERTEXT_COMPONENTS_DESCRIPTION_INFO "Beweeg de muisaanwijzer over de onderdelen om de beschrijving te zien."
  !endif
!endif

!ifdef MUI_DIRECTORYPAGE
  ${LangFileString} MUI_TEXT_DIRECTORY_TITLE "Kies een installatie locatie"
  ${LangFileString} MUI_TEXT_DIRECTORY_SUBTITLE "Kies een map waar U $(^NameDA) wilt installeren."
!endif

!ifdef MUI_UNDIRECTORYPAGE
  ${LangFileString} MUI_UNTEXT_DIRECTORY_TITLE "Kies een deïnstallatie locatie"
  ${LangFileString} MUI_UNTEXT_DIRECTORY_SUBTITLE "Kies een map waar U $(^NameDA) wilt deïnstalleren."
!endif

!ifdef MUI_INSTFILESPAGE
  ${LangFileString} MUI_TEXT_INSTALLING_TITLE "Installeren"
  ${LangFileString} MUI_TEXT_INSTALLING_SUBTITLE "Even gedult terwijl $(^NameDA) wordt geinstalleerd."
  ${LangFileString} MUI_TEXT_FINISH_TITLE "Installatie voltooid"
  ${LangFileString} MUI_TEXT_FINISH_SUBTITLE "De installatie is succesvol afgerond."
  ${LangFileString} MUI_TEXT_ABORT_TITLE "Installatie is afgebroken"
  ${LangFileString} MUI_TEXT_ABORT_SUBTITLE "De installatie is niet voltooid."
!endif

!ifdef MUI_UNINSTFILESPAGE
  ${LangFileString} MUI_UNTEXT_UNINSTALLING_TITLE "DeïnEven geduld terwijl $(^NameDA) wordt gedeïnstalleerd."
  ${LangFileString} MUI_UNTEXT_FINISH_TITLE "Deïnstallatie  voltooid"
  ${LangFileString} MUI_UNTEXT_FINISH_SUBTITLE "Deïnstallatie succesvol afgerond."
  ${LangFileString} MUI_UNTEXT_ABORT_TITLE "Deïnstallatie onderbroken"
  ${LangFileString} MUI_UNTEXT_ABORT_SUBTITLE "Deïnstallatie is niet voltooid."
!endif

!ifdef MUI_FINISHPAGE
  ${LangFileString} MUI_TEXT_FINISH_INFO_TITLE "Voltooien van de $(^NameDA) installatie Wizard"
  ${LangFileString} MUI_TEXT_FINISH_INFO_TEXT "$(^NameDA) is geinstalleerd op uw computer.$\r$\n$\r$\nKlik op einde om de installatie wizard af te sluiten."
  ${LangFileString} MUI_TEXT_FINISH_INFO_REBOOT "Uw computer moet herstarten op de installatie van $(^NameDA) te voltooien. Wilt u nu opnieuw opstarten?"
!endif

!ifdef MUI_UNFINISHPAGE
  ${LangFileString} MUI_UNTEXT_FINISH_INFO_TITLE "Voltooien van de $(^NameDA) deïnstallatie wizard"
  ${LangFileString} MUI_UNTEXT_FINISH_INFO_TEXT "$(^NameDA) is gedeïnstalleerd op uw computer.$\r$\n$\r$\nKlik op einde om de installatie wizard af te sluiten."
  ${LangFileString} MUI_UNTEXT_FINISH_INFO_REBOOT "Uw computer moet herstarten op de deïnstallatie van $(^NameDA)te voltooien. Wilt u nu opnieuw opstarten?"
!endif

!ifdef MUI_FINISHPAGE | MUI_UNFINISHPAGE
  ${LangFileString} MUI_TEXT_FINISH_REBOOTNOW "Nu herstarten"
  ${LangFileString} MUI_TEXT_FINISH_REBOOTLATER "Ik wil later herstarten"
  ${LangFileString} MUI_TEXT_FINISH_RUN "&Starten $(^NameDA)"
  ${LangFileString} MUI_TEXT_FINISH_SHOWREADME "&Toon leesmij"
  ${LangFileString} MUI_BUTTONTEXT_FINISH "&Einde"  
!endif

!ifdef MUI_STARTMENUPAGE
  ${LangFileString} MUI_TEXT_STARTMENU_TITLE "Kies start menu map"
  ${LangFileString} MUI_TEXT_STARTMENU_SUBTITLE "Kies een map in start menu voor de snelkoppeling van $(^NameDA)."
  ${LangFileString} MUI_INNERTEXT_STARTMENU_TOP "Kies een map in start menu waar u de programma snelkoppelingen wilt aanmaken. U kan ook de naam van een nieuwe map opgeven."
  ${LangFileString} MUI_INNERTEXT_STARTMENU_CHECKBOX "Maak geen snelkoppelingen"
!endif

!ifdef MUI_UNCONFIRMPAGE
  ${LangFileString} MUI_UNTEXT_CONFIRM_TITLE "Deïnstalleer$(^NameDA)"
  ${LangFileString} MUI_UNTEXT_CONFIRM_SUBTITLE "Verwijder $(^NameDA) van uw computer."
!endif

!ifdef MUI_ABORTWARNING
  ${LangFileString} MUI_TEXT_ABORTWARNING "Weet u zeker dat U installatie van $(^Name) wilt afbreken?"
!endif

!ifdef MUI_UNABORTWARNING
  ${LangFileString} MUI_UNTEXT_ABORTWARNING "Weet u zeker dat U de deïnstallatie van $(^Name)wilt afbreken?"
!endif

!ifdef MULTIUSER_INSTALLMODEPAGE
  ${LangFileString} MULTIUSER_TEXT_INSTALLMODE_TITLE "Kies gebruikers"
  ${LangFileString} MULTIUSER_TEXT_INSTALLMODE_SUBTITLE "Kies voor welke gebruikers U $(^NameDA) wilt installeren."
  ${LangFileString} MULTIUSER_INNERTEXT_INSTALLMODE_TOP "Kies of U $(^NameDA) alleen voor u zelf of voor alle gebruikers op deze computer wilt installeren. $(^ClickNext)"
  ${LangFileString} MULTIUSER_INNERTEXT_INSTALLMODE_ALLUSERS "Installeer voor iedereen die deze computer gebruikt"
  ${LangFileString} MULTIUSER_INNERTEXT_INSTALLMODE_CURRENTUSER "Installeer alleen voor mij"
!endif
