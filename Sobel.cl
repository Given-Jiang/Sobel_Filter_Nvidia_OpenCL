#define ROWS 576
#define COLS 720


__kernel
__attribute__((task))
void sobel(__global uchar* restrict img_test_in, __global uchar* restrict img_test_out, const unsigned int iterations)
{
    unsigned int count = 0;

    // Filter coefficients
    char Gx[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};
	char Gy[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};

    while(count != iterations)
    {
		char x_dir = 0;
        char y_dir = 0;
		
		if ( count > 2 * COLS + 3)
		{
			#pragma unroll
			for (int i = 0; i < 3 ; ++i)
			{
				#pragma unroll
				for (int j = 0; j < 3; ++j)
				{
					uchar pixel = img_test_in[ i * COLS + j + count ];
					x_dir += pixel * Gx[i][j];
					y_dir += pixel * Gy[i][j]; 
				}
			}
			uchar temp_x = abs(x_dir) + abs(y_dir);
			uchar temp = 0;
			if (temp_x > 127)
			{
				temp = 255;
			}else
			{
				temp = temp_x;
			}
			img_test_out[count++] = temp;
		}else
		{
			img_test_out[count++] = 0;
		}
	}
}


