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

// Calculate m value
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

// Calculate s_m value
void calculate_modified_sums(const int* s, const int* m, int* s_m, int size) {
    #pragma omp parallel for
    for (int i = 0; i < size; i++) {
        s_m[i] = s[i] - (i > 0 ? m[i-1] : 0);
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

int main() {
    int size = 10000;  // Array size
    int num_threads = 16;  // Thread number
    
    omp_set_num_threads(num_threads);
    
    int* x = (int*)malloc(size * sizeof(int));
    int* s = (int*)malloc(size * sizeof(int));
    int* m = (int*)malloc(size * sizeof(int));
    int* s_m = (int*)malloc(size * sizeof(int));
    int* indx = (int*)malloc(size * sizeof(int));
    

    // Generate random input array
    srand(time(NULL)); //ai a sorcaz
    generate_random_array(x, size);

    double start_time = omp_get_wtime();
    
    calculate_prefix_sums(x, s, size);
    calculate_prefix_mins(s, m, size);
    calculate_modified_sums(s, m, s_m, size);
    calculate_index_array(s, m, indx, size);

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
