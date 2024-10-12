#include "InputUtilities.h"
#include <iostream>

int main()
{
	Sleep(2000);

	InputUtilities iu;
	std::cout << iu.SetCursorPos(0, 0);
}