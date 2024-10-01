#include "InputUtilities.h"
#include <iostream>

int main()
{
	Sleep(3000);

	InputUtilities iu;
	std::cout << iu.vkMultiKeyDown({ VK_CONTROL, VK_LWIN, VK_MENU, VK_SHIFT });
	std::cout << iu.MultiKeyDown({ 'l' });
}