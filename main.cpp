#include <stdio.h>
#include <vector>
#include <CL/cl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <tchar.h>
#include <cstdio>
#include <Windows.h> 

//OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using  std::string;
using namespace cv;

//#pragma comment(lib,"OpenCL.lib")

//read image and get arguements
void img_init(string &str, Mat &img_buf, unsigned int &img_rows, unsigned int &img_cols, unsigned int &img_channels)
{
//	img_buf.create(576, 720, CV_8UC1);
	img_buf = imread(str);
	GaussianBlur(img_buf, img_buf, Size(7, 7), 0, 0, BORDER_DEFAULT);
	cvtColor(img_buf, img_buf, CV_BGR2GRAY);
	convertScaleAbs(img_buf, img_buf);  
//	imwrite("test_gray.jpg",img_buf);
	img_rows = img_buf.rows;
	img_cols = img_buf.cols;
	img_channels = img_buf.channels();
}

//create program
cl_program load_program(cl_context context, const char* filename)
{
	std::ifstream in(filename, std::ios_base::binary);
	if(!in.good()) 
	{
		return 0;

	}
	
	// get file length

	in.seekg(0, std::ios_base::end);
    size_t length = in.tellg();
    in.seekg(0, std::ios_base::beg);

    // read program source

	std::vector<char> data(length + 1);
	in.read(&data[0], length);
	data[length] = 0;

    // create and build program 

	const char* source = &data[0];
	cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
	if(program == 0) 
	{
		return 0;
	}
	if(clBuildProgram(program, 0, 0, 0, 0, 0) != CL_SUCCESS) 
	{
		return 0;
	}
	return program;
}


int _tmain(int argc, _TCHAR* argv[])
{

	//begin time counte
	LARGE_INTEGER t1,t2,tc;
	QueryPerformanceFrequency(&tc);
    QueryPerformanceCounter(&t1);

	//Read image
	string str_in_img = "test.jpg";
	Mat in_img;
	unsigned int in_img_rows, in_img_cols, in_img_channels;
	img_init(str_in_img, in_img, in_img_rows, in_img_cols, in_img_channels);

	cout << "in_img_rows = " << in_img_rows <<endl;
	cout << "in_img_cols = " << in_img_cols <<endl;
	cout << "in_img_channels = " << in_img_channels <<endl;
	cout << "isContinue = " << in_img.isContinuous() << endl;

	cl_int err;
    cl_uint num;

	//get platform IDs
    err = clGetPlatformIDs(0, 0, &num);
    if(err != CL_SUCCESS) 
	{
		std::cerr << "Unable to get platforms\n";
        return 0;
    }
    std::vector<cl_platform_id> platforms(num);
    err = clGetPlatformIDs(num, &platforms[0], &num);
    if(err != CL_SUCCESS) 
	{
		std::cerr << "Unable to get platform ID\n";
        return 0;
	}

	//make context
	cl_context_properties prop[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platforms[0]), 0 };
	cl_context context = clCreateContextFromType(prop, CL_DEVICE_TYPE_DEFAULT, NULL, NULL, NULL);
	if(context == 0) 
	{
		std::cerr << "Can't create OpenCL context\n";
		return 0;
	}
	
	//get devices
	size_t cb;
	clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
	std::vector<cl_device_id> devices(cb / sizeof(cl_device_id));
	clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, &devices[0], 0);

	clGetDeviceInfo(devices[0], CL_DEVICE_NAME, 0, NULL, &cb);
	std::string devname;
	devname.resize(cb);
	clGetDeviceInfo(devices[0], CL_DEVICE_NAME, cb, &devname[0], 0);
	std::cout << "Device: " << devname.c_str() << "\n";

	cl_command_queue queue = clCreateCommandQueue(context, devices[0], 0, 0);
	if(queue == 0)
	{
		std::cerr << "Can't create command queue\n";
		clReleaseContext(context);
		return 0;
	}

	//create buffers
	unsigned int DATA_SIZE = in_img_rows * in_img_cols * in_img_channels;
	
	cl_mem cl_img_in = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(uchar) * DATA_SIZE, NULL, NULL);
	cl_mem cl_img_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY,  sizeof(uchar) * DATA_SIZE, NULL, NULL);
	if(cl_img_in == 0 || cl_img_out == 0)
	{
		std::cerr << "Can't create OpenCL buffer\n";
		clReleaseMemObject(cl_img_in);
		clReleaseMemObject(cl_img_out);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);
		return 0;
	}

	//make prograam 
	cl_program program = load_program(context, "..\\Sobel.cl");
	if(program == 0) 
	{
		std::cerr << "Can't load or build program\n";
		clReleaseMemObject(cl_img_in);
		clReleaseMemObject(cl_img_out);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);
		return 0;
	}

	//create kernel
	cl_kernel sobel = clCreateKernel(program, "sobel", 0);
	if(sobel == 0)
	{
		std::cerr << "Can't load kernel\n";
		clReleaseProgram(program);
		clReleaseMemObject(cl_img_in);
		clReleaseMemObject(cl_img_out);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);
		return 0;
	}

	//set kernel arguements
	clSetKernelArg(sobel, 0, sizeof(cl_mem), &cl_img_in);
	clSetKernelArg(sobel, 1, sizeof(cl_mem), &cl_img_out);
	clSetKernelArg(sobel, 2, sizeof(unsigned int), &DATA_SIZE);
//	clSetKernelArg(graying, 3, sizeof(unsigned int), &in_img_cols);
//	clSetKernelArg(graying, 4, sizeof(unsigned int), &in_img_channels);

	//wirte image data to buffer
	uchar* pixel_in = in_img.data;
	err = clEnqueueWriteBuffer(queue, cl_img_in, CL_FALSE, 0, DATA_SIZE, pixel_in, 0, NULL, NULL);
	clFinish(queue);

	//set NDRange 
	size_t work_size = 1;
	cl_event event;
	err = clEnqueueNDRangeKernel(queue, sobel, 1, 0, &work_size, 0, 0, 0, &event);
	clFinish(queue);

	Mat out_img(in_img_rows, in_img_cols, CV_8UC1);//must define the arguements when define the Mat
	cout << "out_isContinue = " << out_img.isContinuous() << endl;
	uchar* pixel_out = out_img.data;
	if(err == CL_SUCCESS)
	{
		err = clEnqueueReadBuffer(queue, cl_img_out, CL_FALSE, 0, DATA_SIZE, pixel_out, 0, 0, 0);
	}
	//time counts over
	QueryPerformanceCounter(&t2);
    printf("OpenCL program Use Time:%f\n",(t2.QuadPart - t1.QuadPart)*1.0/tc.QuadPart);

	//release cl things
	clReleaseKernel(sobel);
	clReleaseProgram(program);
	clReleaseMemObject(cl_img_in);
	clReleaseMemObject(cl_img_out);
	clReleaseCommandQueue(queue);
    clReleaseContext(context);

	//begin time count
	LARGE_INTEGER t3,t4;
	QueryPerformanceFrequency(&tc);
    QueryPerformanceCounter(&t3);

	//opencv comparison
	Mat dst_x,dst_y,dst;
	Sobel(in_img, dst_x, in_img.depth(), 1, 0);  
	Sobel(in_img, dst_y, in_img.depth(), 0, 1);  
	convertScaleAbs(dst_x, dst_x);  
    convertScaleAbs(dst_y, dst_y);  
    addWeighted( dst_x, 0.5, dst_y, 0.5, 0, dst);  
	//time count over
	QueryPerformanceCounter(&t4);
    printf("OpenCV program  Use Time:%f\n",(t4.QuadPart - t3.QuadPart)*1.0/tc.QuadPart);

	//show result
	namedWindow("test_in");
	imshow("test_in",in_img);
	namedWindow("test_in_dst");
	imshow("test_in_dst",dst);
	namedWindow("test_out");
	imshow("test_out",out_img);

	
	waitKey(0);
	return 0;

}

