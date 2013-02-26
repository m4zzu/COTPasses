/*
 * Example for the Modulo Scheduling pass
 *
 * Inspired from Appel Program 20.4, after scalar replacementx
 */
#define N 10

int main(void) {

	int a, b, c, d, e, f, g, h, i, j;

	j = 0;
	b = 1;
	f = 2;
	e = 3;
	g = 4;
	h = 5;

	for(int i = 1; i < N; ++i){
		a = j + b;		// Arithmetic operation using j and b
		b = a + f; 		// Arithmetic operation using a and f
		c = e - j;  	// Arithmetic operation using e and j
		d = f - c;		// Arithmetic operation using f and c
		e = b + d;		// Arithmetic operation using b and d
		f = i / 2;		// Memory read substituted by a div, using i
		g = b * i;		// Memory write substituted by a mul, using b and i
		h = d * i;		// Memory write substituted by a mul, using d and i
		j = i / 3;		// Memory read substituted by a div, using i
	}

	return 0;
}
