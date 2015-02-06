#include <stdio.h>
#include <ctoolx_math/math.h>

#include <netdb.h>
#include <sys/socket.h>

#include <unistd.h>
#include <sys/types.h>

int main() 
{

	
	int i = 10;
	i = fibonacci(10);
	printf("hello:%d\n", i);

	i = 0;
	for ( ; i < 2; i++) {
		fork();
		printf("-\n");
	}

	return 0;
}