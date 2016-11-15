#include "persistence.h"

int main (int argc, char * const argv[])
{
	AJAPersistence persistenceInst;

	bool passTest = persistenceInst.UnitTestDiskReadWrite();
	if(passTest == true)
		printf("passed unit test\n");		
	else 
		printf("failed unit test\n");

    return 0;
}
