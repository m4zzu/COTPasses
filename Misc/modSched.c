// Modulo Scheduling

#define MAX 5

int main(void) {
	int i = 0;
	int exp_2 = 1;
	int fib = 0;
	int fn_1 = 1;
	int fn_2 = 0;

	while (i < MAX) {
		exp_2 = exp_2 * 2;
		fib = fn_1 + fn_2;
		fn_2 = fn_1;
		fn_1 = fib;
		i++;
	}

	return 0;
}
