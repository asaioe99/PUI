# PUI

```
afl-fuzz.exe -i input -o C:\winafl_for_jwc\build32\bin\Release\JWW\output -t 10000 -D C:\Users\***\Documents\DynamoRIO-Windows-8.0.18460\bin32 -- -coverage_module common_lib.dll -coverage_module Jw_win.exe -target_module Jw_win.exe -target_offset 0x0283448 -fuzz_iterations 5000 -nargs 2 -call_convention thiscall -- C:\winafl_for_jwc\build32\bin\Release\JWW\Jw_win.exe @@

```

```
afl-fuzz.exe -i input -o C:\winafl_for_jwc\build32\bin\Release\JWW\output -M M -t 10000 -D C:\Users\***\Documents\DynamoRIO-Windows-8.0.18460\bin32 -- -coverage_module common_lib.dll -coverage_module Jw_win.exe -target_module Jw_win.exe -target_offset 0x0283448 -fuzz_iterations 5000 -nargs 2 -call_convention thiscall -- C:\winafl_for_jwc\build32\bin\Release\JWW\Jw_win.exe @@
```
