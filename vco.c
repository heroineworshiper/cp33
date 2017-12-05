// generate VCO driving curve


#include <stdio.h>
#include <math.h>


#define STEPS 1024
#define MAX 3780

// datasheet
#define EXPONENT 0.6
#define CURVE_WEIGHT 1.25

// experimental
//#define EXPONENT 0.33
//#define CURVE_WEIGHT 0.6

//#define TEST_OUTPUT

int main()
{
	int i, column = 1;
	

#ifndef TEST_OUTPUT	
	printf("const static uint16_t vco_curve[] = {\n");
#endif

	double curve_weight = CURVE_WEIGHT;
	double line_weight = 1.0 - curve_weight;
	int total_points = STEPS * 2;
	double vco_table[STEPS];
	double vco_table2[STEPS];

	for(i = 0; i < STEPS; i++)
	{
		double x = (double)i;
		double y = 0;
		if(x < STEPS / 2)
		{
			double x2 = x * 2;
			y = pow(x2 / (STEPS - 2), EXPONENT) * MAX / 2;
			y = y * curve_weight + x * MAX / STEPS * line_weight;
		}
		else
		{
			double x2 = (STEPS - 1 - x) * 2;
			y = MAX - pow(x2 / STEPS, EXPONENT) * MAX / 2;
			y = y * curve_weight + x * MAX / STEPS * line_weight;
		}

		vco_table[i] = y;
//#ifdef TEST_OUTPUT
//		printf("%f\t%f\n", x * MAX / 1024, y);
//#endif
	}


	for(i = 0; i < STEPS; i++)
	{
		int index = sin(i * 2 * M_PI / STEPS) * STEPS / 2 + STEPS / 2;
		if(index >= STEPS) index = STEPS - 1;
		if(index < 0) index = 0;
//		vco_table2[i] = vco_table[index];
		vco_table2[i] = vco_table[i];
#ifdef TEST_OUTPUT
		printf("%f\n", vco_table2[i]);
#endif
	}


#ifndef TEST_OUTPUT
	for(i = 0; i < total_points; i++)
	{
		double y = 0;
		if(column == 1) printf("\t");
	
		if(i < STEPS)
		{
			y = vco_table2[i];
			printf("%d", (int)y);
		}
		else
		{
			printf("0");
		}


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
#endif


	return 0;
}


