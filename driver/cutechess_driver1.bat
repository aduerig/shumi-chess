cutechess-cli ^
  -engine name=Shumi cmd="C:\programming\shumi-chess\build\bin\Release\shumi_uci - Copy.exe" proto=uci ^
  -engine name=Stockfish cmd="C:\_GoodStuff\stockfish\stockfish-windows-x86-64-avx2.exe" proto=uci option.UCI_LimitStrength=true option.UCI_Elo=2300 ^
  -each tc=40/300 ^
  -games 1 ^
  -concurrency 1 ^
  -pgnout shumi_vs_sf2300.pgn ^
  -debug all > cutechess_all.txt 2>&1