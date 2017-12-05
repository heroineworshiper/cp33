#include <stdio.h>
#include <string.h>
#include <math.h>





double input[] =
{
	0.0000000e+000, 5.7102000e+010,	
	5.0000000e-002, 5.7124000e+010,	
	1.0000000e-001, 5.7147500e+010,	
	1.5000000e-001, 5.7174700e+010,	
	2.0000000e-001, 5.7203900e+010,	
	2.5000000e-001, 5.7237000e+010,	
	3.0000000e-001, 5.7271800e+010,	
	3.5000000e-001, 5.7312200e+010,	
	4.0000000e-001, 5.7356300e+010,	
	4.5000000e-001, 5.7406700e+010,	
	5.0000000e-001, 5.7464000e+010,	
	5.5000000e-001, 5.7527100e+010,	
	6.0000000e-001, 5.7599100e+010,	
	6.5000000e-001, 5.7681200e+010,	
	7.0000000e-001, 5.7771800e+010,	
	7.5000000e-001, 5.7874900e+010,	
	8.0000000e-001, 5.7985700e+010,	
	8.5000000e-001, 5.8103400e+010,	
	9.0000000e-001, 5.8228200e+010,	
	9.5000000e-001, 5.8355100e+010,	
	1.0000000e+000, 5.8483500e+010,	
	1.0500000e+000, 5.8611900e+010,	
	1.1000000e+000, 5.8737600e+010,	
	1.1500000e+000, 5.8864600e+010,	
	1.2000000e+000, 5.8994100e+010,	
	1.2500000e+000, 5.9127200e+010,	
	1.3000000e+000, 5.9269000e+010,	
	1.3500000e+000, 5.9419700e+010,	
	1.4000000e+000, 5.9581800e+010,	
	1.4500000e+000, 5.9755200e+010,	
	1.5000000e+000, 5.9943700e+010,	
	1.5500000e+000, 6.0144100e+010,	
	1.6000000e+000, 6.0352500e+010,	
	1.6500000e+000, 6.0564900e+010,	
	1.7000000e+000, 6.0775000e+010,	
	1.7500000e+000, 6.0969000e+010,	
	1.8000000e+000, 6.1152600e+010,	
	1.8500000e+000, 6.1321900e+010,	
	1.9000000e+000, 6.1477600e+010,	
	1.9500000e+000, 6.1619000e+010,	
	2.0000000e+000, 6.1746400e+010,	
	2.0500000e+000, 6.1865700e+010,	
	2.1000000e+000, 6.1975200e+010,	
	2.1500000e+000, 6.2075300e+010,	
	2.2000000e+000, 6.2171000e+010,	
	2.2500000e+000, 6.2260700e+010,	
	2.3000000e+000, 6.2341500e+010,	
	2.3500000e+000, 6.2417000e+010,	
	2.4000000e+000, 6.2485900e+010,	
	2.4500000e+000, 6.2549700e+010,	
	2.5000000e+000, 6.2608500e+010,	
	2.5500000e+000, 6.2661600e+010,	
	2.6000000e+000, 6.2712500e+010,	
	2.6500000e+000, 6.2757100e+010,	
	2.7000000e+000, 6.2800200e+010,	
	2.7500000e+000, 6.2840000e+010,	
	2.8000000e+000, 6.2879500e+010,	
	2.8500000e+000, 6.2915500e+010,	
	2.9000000e+000, 6.2949100e+010,	
	2.9500000e+000, 6.2980400e+010,	
	3.0000000e+000, 6.3009900e+010	
};

#define STEPS 1024
#define MAX 3780



int main()
{
	int total_in = sizeof(input) / sizeof(double) / 2;
	int total_out = STEPS;
	int i;
	int j;
	
	int output[STEPS * 2];
	bzero(output, sizeof(int) * STEPS * 2);
	double max_freq = input[2 * total_in - 1];
	double min_freq = input[1];
//	printf("%f %f\n", min_freq, max_freq);
	for(i = 0; i < total_out; i++)
	{
		double freq = (double)i * (max_freq - min_freq) / (total_out - 1) + min_freq;
		int input_slot1;
		int input_slot2;
		for(j = 0; j < total_in - 1; j++)
		{
			input_slot1 = j;
			input_slot2 = j + 1;
			if(input[input_slot2 * 2 + 1] >= freq) break;
		}

		double freq1 = input[input_slot1 * 2 + 1];
		double freq2 = input[input_slot2 * 2 + 1];
		double fraction = (freq - freq1) / (freq2 - freq1);

		double value = (1.0 - fraction) * input[input_slot1 * 2] +
			fraction * input[input_slot2 * 2];
		output[i] = (int)(value * MAX / 3.0);
//		printf("%f %d %d %f\n", freq, input_slot1, input_slot2, value);
//		printf("%f %d %d %f\n", fraction, input_slot1, input_slot2, value);
		printf("%d\n", output[i]);
	}
	
	printf("const static uint16_t vco_curve[] = {\n");
	int column = 1;
	int total_points = STEPS * 2;
	for(i = 0; i < total_points; i++)
	{
		if(column == 1) printf("\t");
	
		printf("%d", (int)output[i]);


		if(i < total_points - 1) printf(", ");
		if(column >= 8) 
		{
			printf("\n");
			column = 0;
		}

		column++;
		
	}
	if(column != 1) printf("\n");
	printf("};\n");
	
	
	return 0;
}








