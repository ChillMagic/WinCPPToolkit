@echo off
setlocal EnableDelayedExpansion

set "params=%*"
set "keywords=--build --install --open -P -E --find-package --workflow --help"

for %%i in (%*) do (
    set "param=%%i"
    for %%k in (%keywords%) do (
        if "!param!"=="%%k" (
            endlocal
            set cmake_generate=0
            goto end
        )
    )
)

endlocal
set cmake_generate=1
goto end

:end
