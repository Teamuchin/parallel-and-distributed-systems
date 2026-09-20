#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <limits.h>

// Function to generate random array for testing
void generate_random_array(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = (rand() % 201) - 100;
    }
}

// Function to generate example array for testing
void generate_example_array(int* arr) {
    int example_array[10] = {-2, 1, 3, -7, 11, -2, -6, 12, -3, -1};
    for (int i = 0; i < 10; i++) {
        arr[i] = example_array[i];
    }
}

// Calculate prefix sums array
void calculate_prefix_sums(const int* x, int* s, int size) {
    #pragma omp parallel
    {
        #pragma omp single
        {
            s[0] = x[0];
            
            #pragma omp taskloop
            for (int i = 1; i < size; i++) {
                s[i] = s[i-1] + x[i];
            }
        }
    }
}

// Calculate prefix minimums array
void calculate_prefix_mins(const int* s, int* m, int size) {
    #pragma omp parallel
    {
        #pragma omp single
        {
            m[0] = s[0];
            
            #pragma omp taskloop
            for (int i = 1; i < size; i++) {
                m[i] = (s[i] < m[i-1]) ? s[i] : m[i-1];
            }
        }
    }
}

// Calculate modified sums array (s_m)
void calculate_modified_sums(const int* s, const int* m, int* s_m, int size) {
    #pragma omp parallel for
    for (int i = 0; i < size; i++) {
        s_m[i] = s[i] - (i > 0 ? m[i-1] : 0);
    }
}

// Calculate index array
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

// Find maximum value and its index in array
void find_max_value_and_index(const int* arr, int size, int* max_val, int* max_index) {
    *max_val = arr[0];
    *max_index = 0;
    
    #pragma omp parallel
    {
        int local_max = *max_val;
        int local_max_index = 0;
        
        #pragma omp for
        for (int i = 1; i < size; i++) {
            if (arr[i] > local_max) {
                local_max = arr[i];
                local_max_index = i;
            }
        }
        
        #pragma omp critical
        {
            if (local_max > *max_val) {
                *max_val = local_max;
                *max_index = local_max_index;
            }
        }
    }
}

int main() {
    int num_threads = 2;
    omp_set_num_threads(num_threads);
    
    int* tx = (int*)malloc(10 * sizeof(int));
    int* ts = (int*)malloc(10 * sizeof(int));
    int* tm = (int*)malloc(10 * sizeof(int));
    int* ts_m = (int*)malloc(10 * sizeof(int));
    int* tindx = (int*)malloc(10 * sizeof(int));

    generate_example_array(tx);

    calculate_prefix_sums(tx, ts, 10);
    calculate_prefix_mins(ts, tm, 10);
    calculate_modified_sums(ts, tm, ts_m, 10);
    calculate_index_array(ts, tm, tindx, 10);
    
    // Find example MCS and its indices
    int t_mcs_value, t_mcs_end_index;
    find_max_value_and_index(ts_m, 10, &t_mcs_value, &t_mcs_end_index);

    int t_max_indx = -1;
    for (int i = 0; i <= t_mcs_end_index; i++) {
        if (tindx[i] > t_max_indx) {
            t_max_indx = tindx[i];
        }
    }

    printf("Example MCS Value: %d\n", t_mcs_value);
    printf("Example MCS Range: [%d, %d]\n", t_max_indx + 1, t_mcs_end_index);

    free(tx);   
    free(ts);
    free(tm);
    free(ts_m);
    free(tindx);
    
    int size = 100000;
    
    // Allocate arrays
    int* x = (int*)malloc(size * sizeof(int));
    int* s = (int*)malloc(size * sizeof(int));
    int* m = (int*)malloc(size * sizeof(int));
    int* s_m = (int*)malloc(size * sizeof(int));
    int* indx = (int*)malloc(size * sizeof(int));
    
    // Generate random input array
    srand(time(NULL));
    generate_random_array(x, size);
    
    // Record start time
    double start_time = omp_get_wtime();
    
    // Step 1: Calculate prefix sums
    calculate_prefix_sums(x, s, size);
    
    // Step 2: Calculate prefix minimums
    calculate_prefix_mins(s, m, size);

    // Step 3: Calculate modified sums
    calculate_modified_sums(s, m, s_m, size);

    // Step 4: Calculate index array
    calculate_index_array(s, m, indx, size);

    // Find MCS and its indices
    int mcs_value, mcs_end_index;
    find_max_value_and_index(s_m, size, &mcs_value, &mcs_end_index);
    
    // Find maximum index in indx array
    int max_indx = -1;
    for (int i = 0; i <= mcs_end_index; i++) {
        if (indx[i] > max_indx) {
            max_indx = indx[i];
        }
    }
    
    // Record end time
    double end_time = omp_get_wtime();
    
    // Print results
    printf("MCS Value: %d\n", mcs_value);
    printf("MCS Range: [%d, %d]\n", max_indx + 1, mcs_end_index);
    printf("Execution Time: %f seconds\n", end_time - start_time);
    
    // Free memory
    free(x);
    free(s);
    free(m);
    free(s_m);
    free(indx);
    
    return 0;
}
