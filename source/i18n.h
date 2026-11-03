#pragma once
#include "compatibility.h"
#include <cwchar>
namespace ui {
inline bool russian=false;
inline constexpr const wchar_t* title=L"Grim Dawn — Party Teleport";
inline std::wstring settingsPath;
inline const wchar_t* tr(const wchar_t* en,const wchar_t* ru) {return russian?ru:en;}
inline bool initialize(int argc,wchar_t** argv) {
    wchar_t path[32768];DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);
    if(!n||n>=32768) return false;
    settingsPath=path;settingsPath+=L"\\GrimDawnPartyTeleport";
    if(!CreateDirectoryW(settingsPath.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    settingsPath+=L"\\PartyTeleport.ini";
    wchar_t language[16];GetPrivateProfileStringW(L"General",L"Language",L"en",language,16,settingsPath.c_str());
    russian=!_wcsicmp(language,L"ru");
    for(int i=1;i+1<argc;++i) if(!_wcsicmp(argv[i],L"--language")) {
        if(!_wcsicmp(argv[i+1],L"en")) russian=false;
        else if(!_wcsicmp(argv[i+1],L"ru")) russian=true;
        WritePrivateProfileStringW(L"General",L"Language",russian?L"ru":L"en",settingsPath.c_str());
    }
    return true;
}
inline void toggleLanguage() {
    russian=!russian;
    WritePrivateProfileStringW(L"General",L"Language",russian?L"ru":L"en",settingsPath.c_str());
}
inline const wchar_t* error(const wchar_t* english) {
    if(!russian) return english;
    struct Pair {const wchar_t* en;const wchar_t* ru;};
    static const Pair errors[]={
        {L"Unsupported game version. DLL hashes changed. No changes made. Rebuild the mod for this game version.",L"Версия игры не поддерживается: изменились DLL. Требуется обновление мода. Изменений не внесено."},
        {L"Update ready. Close Grim Dawn normally, then run this launcher again to install it.",L"Обновление готово. Закройте Grim Dawn, затем снова откройте Party Teleport."},
        {L"Could not install the pending update. Close the game and retry.",L"Не удалось установить обновление. Закройте игру и повторите попытку."},
        {L"PartyTeleport.dll is missing beside the launcher.",L"Рядом с приложением отсутствует PartyTeleport.dll. Переустановите мод."},
        {L"Could not create launcher lock.",L"Не удалось создать блокировку запуска приложения."},
        {L"Running x64 Grim Dawn was not found. Start the game through Steam, then open the Party Teleport shortcut.",L"Запущенная Grim Dawn x64 не найдена. Запустите игру через Steam, затем откройте ярлык Party Teleport."},
        {L"Another Grim Dawn process is already running. Use x64 Grim Dawn from this installation. No second copy was started.",L"Запущена другая копия Grim Dawn. Используйте x64 из этой папки игры. Вторая копия не запускалась."},
        {L"Could not start x64 Grim Dawn. Open Steam, launch the game, then use the mod shortcut again.",L"Не удалось запустить Grim Dawn x64. Запустите игру через Steam, затем откройте ярлык мода."},
        {L"The game did not become available. No additional copy will be started. Launch x64 Grim Dawn through Steam and retry.",L"Игра не стала доступна. Запустите Grim Dawn x64 через Steam и повторите попытку."},
        {L"Cannot open the game process. Run the game and launcher at the same permission level.",L"Не удалось открыть процесс игры. Запускайте игру и Party Teleport с одинаковыми правами."},
        {L"Could not allocate the DLL path in the game process.",L"Не удалось выделить память для пути к DLL в процессе игры."},
        {L"Could not write the DLL path.",L"Не удалось записать путь к DLL в процесс игры."},
        {L"Could not load the mod DLL into the game.",L"Не удалось загрузить DLL мода в игру."},
        {L"DLL loading timed out. Restart the game before retrying.",L"Истекло время загрузки DLL. Перезапустите игру перед повторной попыткой."},
        {L"The game did not load the DLL. Check that 64-bit Grim Dawn is still running.",L"Игра не загрузила DLL. Проверьте, что Grim Dawn x64 ещё запущена."}
    };
    for(const auto& pair:errors) if(!wcscmp(pair.en,english)) return pair.ru;
    return english;
}
}
