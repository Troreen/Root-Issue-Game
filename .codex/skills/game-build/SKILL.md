---
name: game-build
description: Build and run this repository's Game.sln in Debug, Release, or Retail using the bundled PowerShell scripts. Use when Codex needs to compile the game, launch GameMain for smoke tests, or quickly switch build configuration during iteration.
---

# Game Build Skill

Use this skill to build or run the game from this repository root.

## Build Workflow

1. Build with the bundled script:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .codex/skills/game-build/scripts/build-game.ps1 -Configuration Debug
```

2. Swap `Debug` for `Release` or `Retail` when requested.
3. Rebuild instead of incremental build when needed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .codex/skills/game-build/scripts/build-game.ps1 -Configuration Retail -Rebuild
```

## Run Workflow

1. Run the game executable for the requested configuration:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .codex/skills/game-build/scripts/run-game.ps1 -Configuration Debug
```

2. Pass additional game args after the configuration if needed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .codex/skills/game-build/scripts/run-game.ps1 -Configuration Release -- -SomeFlag
```

## Notes

- Build script resolves `MSBuild.exe` through `vswhere` and targets `Game.sln`.
- Run script launches `Bin/GameMain_<Configuration>.exe`.
- If run fails due to missing executable, build that configuration first.
