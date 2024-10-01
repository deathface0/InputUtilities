#include "InputUtilities.h"
#include <iostream>

int main()
{
	Sleep(2000);

	InputUtilities iu;
	std::cout << iu.mappedKeyDown(VK_OEM_COMMA);
	/*std::cout << iu.MultiKeyDown({ VK_CONTROL, VK_LWIN, VK_MENU, VK_SHIFT, 'l'});
	std::cout << iu.MultiKeyUp({ VK_CONTROL, VK_LWIN, VK_MENU, VK_SHIFT, 'l'});*/
}