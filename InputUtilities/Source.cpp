#include "InputUtilities.h"
#include <iostream>

int main()
{
	//MouseUtilities::move(30, 30);
	Sleep(1000);
	std::cout << KeyboardUtilities::virtualCombinedInput({VK_CONTROL, VK_MENU, VK_DELETE});
}