#include "InputUtilities.h"
#include <iostream>

int main()
{
	Sleep(2000);

	InputUtilities iu;

	for (char i = 32; i <= 126; i++)
	{
		iu.Key(iu.BuildVKey(i));
	}	
}