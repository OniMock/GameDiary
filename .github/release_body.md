## 🎮 GameDiary PSP - CODENAME_PLACEHOLDER

New version available.

---

## 📦 Download

- **GameDiary.zip** (PRX + EBOOT) - Standard release.
  https://github.com/REPO_PLACEHOLDER/releases/download/TAG_PLACEHOLDER/GameDiary.zip

- **GameDiaryDebug.zip** - Debug version. Use this **only** if the main version has issues.
  https://github.com/REPO_PLACEHOLDER/releases/download/TAG_PLACEHOLDER/GameDiaryDebug.zip
  - **Why use it?** It generates detailed step-by-step text logs in `ms0:/PSP/COMMON/GameDiary/debug-DD-MM-YYYY.txt`.
  - **When is it useful?** If the plugin fails to read a Game ID, crashes, or playtime isn't saving, these logs will tell us exactly where and why it failed.

---

## 📦 Installation

### 📌 Plugin (Tracker)

1. Copy the `GameDiary.prx` file to your `SEPLUGINS` folder on your memory stick (`ms0:/SEPLUGINS/` or `ef0:/SEPLUGINS/` for PSP Go).

2. Enable the plugin:
   - **For ARK-4**:

     > ⚠️ **PSP Go Users**
     > If you are using the PSP Go internal storage, replace `ms0:/` with `ef0:/` in all paths.
     > Open `ms0:/SEPLUGINS/plugins.txt` (or `ef0:/SEPLUGINS/plugins.txt`) in a text editor and add:

     ```
     psp, GameDiary.prx, on
     ```

     To also enable PS1 games tracking, add:

     ```
     ps1, GameDiary.prx, on
     ```

     To enable Homebrew tracking, add:

     ```
     game, GameDiary.prx, on
     ```

   - **For other CFWs**:

     > ⚠️ **PSP Go Users**
     > Replace `ms0:/` with `ef0:/` in all paths.
     - Enable for PSP games and Homebrew by adding to `ms0:/seplugins/game.txt`:

     ```
     ms0:/seplugins/GameDiary.prx 1
     ```

     - Enable for PS1 games by adding to `ms0:/seplugins/pops.txt`:

     ```
     ms0:/seplugins/GameDiary.prx 1
     ```

3. Restart your PSP (or reset VSH).

---

### 🎮 Application (EBOOT)

1. Copy the `GameDiary` folder to:

```
/PSP/GAME/
```

2. Final structure:

```
PSP/
  GAME/
    GameDiary/
      EBOOT.PBP
```

---

## 📝 Changelog

CHANGELOG_PLACEHOLDER
