#include <czmq.h>
#include "types.h"

zsock_t *responder;

void convert_to_struct(zframe_t *frame)
{
	char *str = zframe_strdup(frame);
	if (str[0] == 1)
	{
		one_t a;
		memcpy(&a, str + 1, sizeof(one_t));
		printf("Received one_t: %d %d\n", a.a, a.b);
	}
	else if (str[0] == 2)
	{
		two_t a;
		memcpy(&a, str + 1, sizeof(two_t));
		printf("Received two_t: %f %c %c\n", a.a, a.b, a.c);
	}
	else if (str[0] == 3)
	{
		three_t a;
		memcpy(&a, str + 1, sizeof(three_t));
		printf("Received three_t: %c %f %d\n", a.a, a.b, a.c);
	}
	else
	{
		printf("Received unknown struct");
	}
	free(str);
}

int main()
{
	zsock_t *responder = zsock_new_pull("tcp://*:5555");

	while (1)
	{
		zframe_t *frame = zframe_recv(responder);
		if (!frame)
		{
			break;
		}
		convert_to_struct(frame);
		zframe_destroy(&frame);
	}
	zsock_destroy(&responder);
	return 0;
}
