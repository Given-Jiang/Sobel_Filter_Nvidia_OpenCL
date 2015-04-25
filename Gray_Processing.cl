
__kernel void graying(__global const uchar* restrict img_test_in, __global uchar* restrict img_test_out, const unsigned int iterations)
{
    unsigned int count = 0; 
    uchar rows[3];
    while(count != iterations)
    {
		img_test_out[count] = img_test_in[count];
		count++;
    }
}


