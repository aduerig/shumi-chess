@echo off

"C:\Program Files (x86)\Cute Chess\cutechess-cli.exe" ^
  -engine name=Shumi cmd="C:\programming\shumi-chess\build\bin\Release\shumi_uci - Copy.exe" proto=uci stderr="shumi-stderr.txt" ^
  -engine name=Arasan cmd="C:\Program Files\Arasan\25.4\arasanx-64.exe" proto=uci stderr="arasan-stderr.txt" ^
  -each tc=40/5+0.1 ^
  -games 1 ^
  -rounds 1 ^
  -pgnout "cutechess-test.pgn" ^
  > "cutechess-output.txt" 2>&1
