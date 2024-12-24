#include <conio.h>
#include "AudioStorage.hpp"


int main() 
{
    setlocale(LC_ALL, "rus");
    AudioStorage audioStorage;
    audioStorage.initRecord();
    audioStorage.startRecord();

    while (true) {
        if (_kbhit()) {
            char c = _getch();
            if (c == 'q') break;
        }
    }

    audioStorage.stopRecord();

    system("pause");
    return 0;
}
