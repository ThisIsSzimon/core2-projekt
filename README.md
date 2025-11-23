# Templatka do projektu CORE2
## Konfiguracja CMake
1. Całość projektu pobieramy i umieszczamy w swoim folderze (dla przykładu C:\Users\szymo\Desktop\core2-projekt), po czym do niego wchodzimy.
``` powershell
cd "C:\Users\szymo\Desktop\core2-projekt"
```

2. Tworzymy katalog "build" w którym będzie budował się projekt w tym plik .hex
``` powershell
mkdir build
cd build
```

3. Ważne jest posiadania plików z "husarion.husarion-1.5.30" ponieważ to będzie nasz szkielet projektu. Następnie ustawiamy CMake:
``` powershell
cmake .. -G Ninja -DCMAKE_MAKE_PROGRAM=ninja `
  -DPORT=stm32 `
  -DBOARD_VERSION="1.0.0" `
  -DBOARD_TYPE=core2 `
  -DHFRAMEWORK_PATH="C:/Users/szymo/Desktop/husarion.husarion-1.5.30/sdk"
```

4. Poleceniem ninja budujemy projekt BĘDĄC W KATALOGU BUILD
``` powershell
cd "C:\Users\szymo\Desktop\core2-projekt\build"
ninja
```

5. Po poprawnym zbudowaniu projektu możemy użyć core2-flasher.exe dedykowanego dla windowsa
``` powershell
cd "C:\Users\szymo\Desktop\core2-projekt\build"
& "C:\Users\szymo\Desktop\husarion.husarion-1.5.30\sdk\tools\win\core2-flasher.exe" myproject.hex
```

6. Możliwe że będzie problem z toolchainem od arm-gcc. Należałoby wtedy pobrać starszą wersję np. arm-none-eabi-gcc 7-2018-q2 i uwzględnić to w zmiennych środowiskowych
