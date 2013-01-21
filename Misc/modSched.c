// Modulo Scheduling

#define MAX 5

int main(void) {
	int i = 0;
	int exp_2 = 1;
	int fib = 0;
	int fn_1 = 1;
	int fn_2 = 0;
	
	while (i < MAX) {
		fib = fn_1 + fn_2;
		fn_2 = fn_1;
		fn_1 = fib;
		i++;
		exp_2 = exp_2 * 2;
	}

	return 0;
}

/*
Suggerimento da Marco Minutoli: 
provare a mettere una chiamata a funzione per avere due basic blocks nel loop body

int sum (int a, int b);

int pippo(){

	int a = 0;

	int b = 0

	while (true) {
		++a;
		b = sum (a, b);
		a = 2 * a;
	}
}

*/