#include <stdio.h>
#include <stdint.h>







int main(int argc, char *argv[])
{
	FILE *fd = fopen(argv[1], "r");
	fseek(fd, 44 + 2, SEEK_SET);

	int total_samples = 0;
	int sample_period = 4410;
	int history_size = 32;
	int history[history_size];
	int i;
	int crossings = 0;
	int threshold = 100;
	int time_threshold = 32;
	int crossing_sample = 0;
	int max = 0;
	int min = 0;
	while(!feof(fd))
	{
		int16_t sample;
		fread(&sample, 2, 1, fd);
		if(!feof(fd))
			fseek(fd, 2, SEEK_CUR);
//		printf("%d\n", sample);
		for(i = 0; i < history_size - 1; i++)
		{
			history[i] = history[i + 1];
		}
		history[history_size - 1] = sample;
		crossing_sample++;
		
		if(history[0] < -threshold && sample >= threshold &&
			crossing_sample >= time_threshold)
		{
			crossing_sample = 0;
			crossings++;
		}
		
		if(sample > max) max = sample;
		if(sample < min) min = sample;
		total_samples++;
		if(!(total_samples % sample_period))
		{
			printf("%d %d %d %d\n", total_samples, crossings * 10, min, max);
			crossings = 0;
			max = 0;
			min = 0;
		}
	}
}






