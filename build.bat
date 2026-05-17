@echo off

set CFLAGS=-Wall -Wextra -Wunused --std=c++17
set LIBS=-lraylib -lopengl32 -lgdi32 -lwinmm 

g++ %CFLAGS% -I include/ -c src/main.cpp -o objs/main.o
g++ objs/main.o -o main.exe %CFLAGS% -I include/ -L lib/ %LIBS%

:: Final exe
IF "%1" == "release" (
    g++ -mwindows -O3 --std=c++17 -I include/ -c src/main.cpp -o objs/main.o 
    g++ objs/main.o ./icon_res/resources.res -o A-Star.exe -mwindows -O3 --std=c++17 -I include/ -L lib/ %LIBS%
)
