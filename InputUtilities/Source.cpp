#include "InputUtilities.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
	Sleep(2000);
	InputUtilities iu;

	iu.scMultiKey({ VK_LWIN, 'r'});
}