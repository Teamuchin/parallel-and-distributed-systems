#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <limits.h>

// Generate randomized array of desired size between -100 and 100
void generate_random_array(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = (rand() % 201) - 100;
    }
}

// Calculate s value
void calculate_prefix_sums(const int* x, int* s, int size) {
    s[0] = x[0];
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp taskloop
            for (int i = 1; i < size; i++) {
                s[i] = s[i-1] + x[i];
            }
        }
    }
}

// Calculate m value
void calculate_prefix_mins(const int* s, int* m, int size) {
    m[0] = s[0];
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp taskloop
            for (int i = 1; i < size; i++) {
                m[i] = (s[i] < m[i-1]) ? s[i] : m[i-1];
            }
        }
    }
}

// Calculate s_m value
void calculate_modified_sums(const int* s, const int* m, int* s_m, int size) {
    s_m[0] = s[0];  // Initialize first element
    
    #pragma omp parallel for
    for (int i = 1; i < size; i++) {
        s_m[i] = s[i] - m[i-1];
    }
}

// Calculate indx
void calculate_index_array(const int* s, const int* m, int* indx, int size) {
    #pragma omp parallel for
    for (int j = 0; j < size; j++) {
        indx[j] = -1;
        for (int i = 0; i <= j; i++) {
            if (s[i] == m[j]) {
                indx[j] = i;
            }
        }
    }
}

// Find mcs and its max value index
void find_max_value_and_index(const int* smArr, int size, int* max_val, int* max_index) {
    *max_val = smArr[0];
    *max_index = 0;
    
    #pragma omp parallel
    {
        int local_max = *max_val;
        int local_max_index = 0;
        
        #pragma omp for
        for (int i = 1; i < size; i++) {
            if (smArr[i] > local_max) {
                local_max = smArr[i];
                local_max_index = i;
            }else if (smArr[i] == local_max && i < local_max_index) {
                local_max_index = i;
            }
        }
        
        #pragma omp critical
        {
            if (local_max > *max_val) {
                *max_val = local_max;
                *max_index = local_max_index;
            }else if(local_max == *max_val && local_max_index < *max_index){
                *max_index = local_max_index;
            }
        }
    }
}
void find_mcs_start_index(const int* indx, const int max_index, int* start_index) {
    *start_index = 0;
    
    for (int i = 0; i <= max_index; i++) {
        if (indx[i] != -1) {
            *start_index = indx[i];
            break;
        }
    }
}

int main() {
    int num_threads = 16;  // Thread number
    omp_set_num_threads(num_threads);


    // Test with example array first
    int ex_size = 10;
    int ex_array[] = {-2, 1, 3, -7, 11, -2, -6, 12, -3, -1};

    int* ex_s = (int*)malloc(ex_size * sizeof(int));
    int* ex_m = (int*)malloc(ex_size * sizeof(int));
    int* ex_s_m = (int*)malloc(ex_size * sizeof(int));
    int* ex_indx = (int*)malloc(ex_size * sizeof(int));

    // Calculate all arrays
    calculate_prefix_sums(ex_array, ex_s, ex_size);
    calculate_prefix_mins(ex_s, ex_m, ex_size);
    calculate_modified_sums(ex_s, ex_m, ex_s_m, ex_size);
    calculate_index_array(ex_s, ex_m, ex_indx, ex_size);

    // Find MCS
    int ex_mcs_value, ex_mcs_end_index, ex_mcs_start_index;
    find_max_value_and_index(ex_s_m, ex_size, &ex_mcs_value, &ex_mcs_end_index);
    find_mcs_start_index(ex_indx, ex_mcs_end_index, &ex_mcs_start_index);

    printf("Example array MCS Value: %d\n", ex_mcs_value);
    printf("Example array MCS Range: [%d, %d]\n", ex_mcs_start_index + 1, ex_mcs_end_index);

    // Free memory
    free(ex_s);
    free(ex_m);
    free(ex_s_m);
    free(ex_indx);
















    int size = 100000;  // Array size
    
    

    int* x = (int*)malloc(size * sizeof(int));
    int* s = (int*)malloc(size * sizeof(int));
    int* m = (int*)malloc(size * sizeof(int));
    int* s_m = (int*)malloc(size * sizeof(int));
    int* indx = (int*)malloc(size * sizeof(int));
    
    // Generate random input array
    srand(time(NULL)); //select seed randomizer as time
    generate_random_array(x, size);

    double start_time = omp_get_wtime();
    
    calculate_prefix_sums(x, s, size);
    calculate_prefix_mins(s, m, size);
    calculate_modified_sums(s, m, s_m, size);
    calculate_index_array(s, m, indx, size);

    int mcs_value, mcs_end_index, mcs_start_index;

    find_max_value_and_index(s_m, size, &mcs_value, &mcs_end_index);

    find_mcs_start_index(indx, mcs_start_index, &mcs_end_index);
    
    double end_time = omp_get_wtime();
    
    printf("MCS Value: %d\n", mcs_value);
    printf("MCS Range: [%d, %d]\n", mcs_start_index + 1, mcs_end_index);
    printf("Execution Time: %f seconds\n", end_time - start_time);
    
    free(x);
    free(s);
    free(m);
    free(s_m);
    free(indx);
    
    return 0;
}
