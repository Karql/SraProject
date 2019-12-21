TASKKILL /F /IM RobotSoccer.exe
TASKKILL /F /IM ConsoleDebugger.exe

DEL C:\Strategy\blue\Team1.dll
COPY C:\Projects\SraProject\Debug\Strategy.dll C:\Strategy\blue\
REN C:\Strategy\blue\Strategy.dll Team1.dll

DEL C:\Strategy\yellow\Team2.dll
COPY C:\Projects\SraProject\Debug\Strategy.dll C:\Strategy\yellow\
REN C:\Strategy\yellow\Strategy.dll Team2.dll

START C:\Strategy\ConsoleDebugger.exe
START C:\Strategy\RobotSoccer.exe