## Grim Dawn — Party Teleport 0.4.0 (experimental)

- Overlay clicks and wheel events are filtered in the supported game's DirectInput mouse reader. Consumed presses stay blocked until release, including a drag outside the overlay.
- Settings now open and close with **right-click** on the overlay. **Done** also closes settings. No F-key hotkeys remain.
- Wider Verdana text and wider, two-line teammate buttons.
- A teammate in another named area shows the game's localized area name instead of distance, using the multiplayer heartbeat. Missing names show **Location unavailable**.
- A small Party Teleport tile keeps settings accessible when there is no party.
- Updated English/Russian READMEs and previews, with mouse filtering, area-label and right-click tests.

Start Grim Dawn x64 through Steam, then open **Grim Dawn — Party Teleport**. Use windowed or borderless mode. Close the game and app before updating. The supported DirectInput.dll hash is now checked alongside Game.dll and Engine.dll.

**Experimental:** automated tests cover the new behavior; live mouse handling, remote area labels, multiplayer teleportation, synchronization and compatibility with other injected mods still need in-game verification. Area names follow the game's language, while overlay controls follow the selected English/Russian setting.

### Русский

Настройка открывается и закрывается **правым кликом по оверлею**, кнопка **Готово** также её закрывает. F-клавиши удалены. Добавлена фильтрация мыши внутри игрового DirectInput, чтобы нажатия и колесо над оверлеем не вызывали игровые действия. Поглощённое нажатие блокируется до отпускания.

Текст стал шире, кнопки — двухстрочными. Для товарища в другой именованной локации вместо расстояния показывается её название на языке игры; при отсутствии данных — **Локация неизвестна**. Без группы остаётся плитка для доступа к настройке. Обновлены README и тесты. Перед обновлением закройте игру и приложение. Поведение в живой игре ещё требуется проверить.
