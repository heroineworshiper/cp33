#include <stdio.h>
#include <math.h>


#define N_WAVE 16384

int main()
{
	int i;
	printf("static const int16_t SINEWAVE[] = {\n\t");

	for(i = 0; i < N_WAVE; i++)
	{
		int value = (int)(32767 * sin(i * (M_PI * 2) / N_WAVE));
		printf("%d", value);
		if(i < N_WAVE - 1) printf(", ");
		if(!((i + 1) % 8) && i < N_WAVE - 1) printf("\n\t");
	}
	printf("\n};\n");
}






