mkdir _build

rmdir /s /q genEFU\spiffs
mkdir genEFU\spiffs
xcopy /s /q /y data\* genEFU\spiffs\.

genEFU\bin\win\mkspiffs.exe -c data/ -p 256 -b 8192 -s 1028096 genEFU/firmware/spiffs.bin
genEFU\bin\win\esptool.exe -cd none -cb 115200 -cp COM4 -ca 0x000000 -cf genEFU/firmware/XMC.ino.bin -ca 0x300000 -cf genEFU/firmware/spiffs.bin