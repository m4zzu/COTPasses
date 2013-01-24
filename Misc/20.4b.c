// Modulo Scheduling
// Example taken from Appel

#define N 5

int main(void) {

	int a, b, c, d, e, f, j;

	j = 0;
	b = 1;
	f = 2;
	e = 3;
	
	for(int i = 1; i < N; ++i){
		a = j + b;
		b = a + f;
		c = e - j;
		d = f - c;
		e = b + d;
		f = i / 2;
		j = i * 4;
	}

	return 0;
}
