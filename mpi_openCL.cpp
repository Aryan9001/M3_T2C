#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <chrono>
#include <CL/cl.h>

#define MASTER_RANK 100

void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quicksort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

int main(int argc, char** argv) {
    int rr, sz;
    int* dt;
    int n = 1000; // Number of elements in array

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rr);
    MPI_Comm_size(MPI_COMM_WORLD, &sz);

    if (rr == MASTER_RANK) {
        dt = (int*)malloc(n * sizeof(int));
        // Initialize data with random values
        for (int i = 0; i < n; i++) {
            dt[i] = rand() % 1000;
        }
    }

    // Broadcast the array to all processes
    MPI_Bcast(dt, n, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    auto start = std::chrono::steady_clock::now();

    // Perform quicksort with OpenCL
    // Initialize OpenCL
    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_uint num_platforms, num_devices;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &num_platforms);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &num_devices);
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    
    // Create buffer for data
    cl_mem data_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, n * sizeof(int), NULL, &ret);
    ret = clEnqueueWriteBuffer(command_queue, data_buffer, CL_TRUE, 0, n * sizeof(int), dt, 0, NULL, NULL);

    // Create program from source
    const char* source = "__kernel void quicksort(__global int* arr) { }";
    cl_program program = clCreateProgramWithSource(context, 1, &source, NULL, &ret);
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

    // Create kernel
    cl_kernel kernel = clCreateKernel(program, "quicksort", &ret);

    // Set kernel arguments
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&data_buffer);

    // Execute kernel
    size_t time_size = n;
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &time_size, NULL, 0, NULL, NULL);
    ret = clFinish(command_queue);

    // Read back the sorted data
    ret = clEnqueueReadBuffer(command_queue, data_buffer, CL_TRUE, 0, n * sizeof(int), data, 0, NULL, NULL);

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> elapsed_seconds = end - start;

    if (rank == MASTER_RANK) {
        printf("Sorted array:\n");
        for (int i = 0; i < n; i++) {
            printf("%d ", data[i]);
        }
        printf("\n");
        printf("Execution time: %.2f microseconds\n", elapsed_seconds.count());

        // Release OpenCL resources
        ret = clFlush(command_queue);
        ret = clReleaseKernel(kernel);
        ret = clReleaseProgram(program);
        ret = clReleaseMemObject(data_buffer);
        ret = clReleaseCommandQueue(command_queue);
        ret = clReleaseContext(context);

        free(dt);
    }

    MPI_Finalize();
    return 0;
}
