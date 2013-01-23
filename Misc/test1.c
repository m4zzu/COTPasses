// Modulo Scheduling - First test

#define MAX 8

int main(void) {
	int i = 0;
	int a = 1;
	int b = 0;
	
	while (i < MAX) {
		i = a + b;
		b = b + 1;
		a = a*4;
	}

	return 0;
}
