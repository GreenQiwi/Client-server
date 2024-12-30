#include <conio.h>
#include "AudioStorage.hpp"


int main() 
{
    setlocale(LC_ALL, "rus");
    AudioStorage audioStorage;
    audioStorage.InitRecord();
    audioStorage.StartRecord();

    while (true) {
        if (_kbhit()) {
            char c = _getch();
            if (c == 'q') break;
        }
    }

    audioStorage.StopRecord();

    system("pause");
    return 0;
}
