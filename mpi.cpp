#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <chrono>

#define MASTER 1000

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

    if (rr == MASTER) {
        dt = (int*)malloc(n * sizeof(int));
        // Initialize data with random values
        for (int i = 0; i < n; i++) {
            dt[i] = rand() % 1000;
        }
    }

    // Broadcast the array to all processes
    MPI_Bcast(dt, n, MPI_INT, MASTER, MPI_COMM_WORLD);

    auto start = std::chrono::steady_clock::now();

    // Perform quicksort
    quicksort(dt, 0, n - 1);

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> elapsed_seconds = end - start;

    if (rr == MASTER) {
        printf("Sorted array:\n");
        for (int i = 0; i < n; i++) {
            printf("%d ", data[i]);
        }
        printf("\n");
        printf("Execution time: %.2f microseconds\n", elapsed_seconds.count());
        free(dt);
    }

    MPI_Finalize();
    return 0;
}
