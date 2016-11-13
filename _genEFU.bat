mkdir _build

rmdir /s /q genEFU\spiffs
mkdir genEFU\spiffs
xcopy /s /q /y data\* genEFU\spiffs\.

cd genEFU
ESPSFlashTool_XMC.jar
