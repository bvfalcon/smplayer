﻿;Language: Hungarian (1038)
;Hungarian language strings for the Windows SMPlayer NSIS installer.
;
;Save file as UTF-8 w/ BOM
;

!insertmacro LANGFILE "Hungarian" "Magyar"

; Startup
${LangFileString} Installer_Is_Running "A telepítő már fut."
${LangFileString} Installer_No_Admin "Rendszergazdaként kell bejelentkeznie a program telepítéséhez."
${LangFileString} SMPlayer_Is_Running "An instance of SMPlayer is running. Please exit SMPlayer and try again."

${LangFileString} OS_Not_Supported "Unsupported operating system.$\nSMPlayer ${SMPLAYER_VERSION} requires at least Windows XP and may not work correctly on your system.$\nDo you really want to continue with the installation?"
${LangFileString} Win64_Required "A 64-bit Windows operating system is required to install this software."
${LangFileString} Existing_32bitInst "An existing 32-bit installation of SMPlayer exists. You must uninstall 32-bit SMPlayer first."
${LangFileString} Existing_64bitInst "An existing 64-bit installation of SMPlayer exists. You must uninstall 64-bit SMPlayer first."

; Welcome page
${LangFileString} WelcomePage_Title "$(^NameDA) Setup"
${LangFileString} WelcomePage_Text "Setup will guide you through the installation of $(^NameDA).$\r$\n$\r$\nIt is recommended that you close all instances of SMPlayer before starting setup. This will make it possible to update relevant program files without having to reboot your computer.$\r$\n$\r$\n$_CLICK"

; Components Page
${LangFileString} ShortcutGroupTitle "Shortcuts"
${LangFileString} MPlayerGroupTitle "MPlayer Components"

${LangFileString} Section_SMPlayer "SMPlayer (required)"
${LangFileString} Section_SMPlayer_Desc "SMPlayer, shared libraries, and documentation."

${LangFileString} Section_DesktopShortcut "Desktop"
${LangFileString} Section_DesktopShortcut_Desc "Creates a shortcut to SMPlayer on the desktop."

${LangFileString} Section_StartMenu "Start Menu"
${LangFileString} Section_StartMenu_Desc "Create a Start Menu entry for SMPlayer."

${LangFileString} Section_MPlayer "MPlayer (required)"
${LangFileString} Section_MPlayer_Desc "MPlayer; required for playback."

${LangFileString} Section_MPlayerCodecs "Binary Codecs"
${LangFileString} Section_MPlayerCodecs_Desc "Optional codecs for MPlayer. (Internet Connection required for installation)"

${LangFileString} Section_MEncoder_Desc "A companion program to MPlayer that can be used to encode or transform supported audio or video streams."

${LangFileString} Section_IconThemes "Icon Themes"
${LangFileString} Section_IconThemes_Desc "Additional icon themes for SMPlayer."

${LangFileString} Section_Translations "Languages"
${LangFileString} Section_Translations_Desc "Non-English language files for SMPlayer."

${LangFileString} MPlayer_Codec_Msg "A bináris kodek csomagok támogatást nyújtanak natívan még nem támogatott kodekekhez, mint pl. az újabb RealVideo variánsok és sok ritka formátum.$\nNem szükségesek a legtöbb gyakori formátum lejátszásához, mint a DVD-k, MPEG-1/2/4, stb."

; Upgrade/Reinstall Page
${LangFileString} Reinstall_Header_Text "Select Install Type"
${LangFileString} Reinstall_Header_SubText "Select Overwrite or Uninstall mode."

${LangFileString} Reinstall_Msg1 "You have an existing installation of SMPlayer in the following folder:"
${LangFileString} Reinstall_Msg2 "Please select how to proceed:"
${LangFileString} Reinstall_Overwrite "Overwrite ($Inst_Type) the existing installation"
${LangFileString} Reinstall_Uninstall "Uninstall (remove) the existing installation"
${LangFileString} Reinstall_Msg3_1 "Click Start when ready to proceed."
${LangFileString} Reinstall_Msg3_2 "Click Next when ready to proceed."
${LangFileString} Reinstall_Msg3_3 "Click Uninstall when ready to proceed."
${LangFileString} Reinstall_Msg4 "Change Installation Settings"

${LangFileString} Type_Reinstall "reinstall"
${LangFileString} Type_Downgrade "downgrade"
${LangFileString} Type_Upgrade "upgrade"

${LangFileString} StartBtn "Start"

; Codecs Section
${LangFileString} Codecs_DL_Msg "MPlayer kodekek letöltése..."
${LangFileString} Codecs_DL_Retry "Az MPlayer kodekek telepítése nem sikerült. Újra próbáljam?"
${LangFileString} Codecs_DL_Failed "Az MPlayer kodekek letöltése nem sikerült: '$R0'."
${LangFileString} Codecs_Inst_Failed "Az MPlayer kodekek telepítése nem sikerült."

; Uninstaller
${LangFileString} Uninstaller_No_Admin "A program eltávolításához rendszergazda jogosultság szükséges."
${LangFileString} Uninstaller_Aborted "Az eltávolítást a felhasználó megszakította."
${LangFileString} Uninstaller_NotInstalled "Nem úgy néz ki, mint ha az SMPlayer ebbe a könyvtárba lett volna telepítve: '$INSTDIR'.$\r$\nMégis folytassam (nem ajánlott)?"
${LangFileString} Uninstaller_64bitOnly "This installation can only be uninstalled on 64-bit Windows."

; Vista & Later Default Programs Registration
${LangFileString} Application_Description "Az SMPlayer egy komplett felület az MPlayerhez, mindent támogat az alap funkcióktól kezdve, mint a videók, DVDk, VCDk lejátszása, haladó funkciókig, mint az MPlayer szűrők, edl listák és még sok más."

; Misc
${LangFileString} Info_Codecs_Backup "Backing up codecs from previous installation..."
${LangFileString} Info_Codecs_Restore "Restoring codecs from previous installation..."
${LangFileString} Info_Del_Files "Deleting Files..."
${LangFileString} Info_Del_Registry "Deleting Registry Keys..."
${LangFileString} Info_Del_Shortcuts "Deleting Shortcuts..."
${LangFileString} Info_Rest_Assoc "Restoring file associations..."
${LangFileString} Info_RollBack "Rolling back changes..."
${LangFileString} Info_Files_Extract "Extracting files..."
