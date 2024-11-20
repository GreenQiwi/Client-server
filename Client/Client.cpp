#include <conio.h>
#include "AudioStorage.hpp"


int main() 
{

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
