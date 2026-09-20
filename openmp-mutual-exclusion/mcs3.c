#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <limits.h>

// Generate randomized array of desired size between negative value size of array and positive value size of array
void generate_random_array(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = (rand() % (size*2+1)) - size;
    }
}

// Calculate s value
void calculate_s(const int* x, int* s, int size) {
    s[0] = x[0];
    for (int i = 1; i < size; i++) {
        s[i] = s[i-1] + x[i];
    }
}

// Calculate m value
void calculate_m(const int* s, int* m, int size) {
    m[0] = s[0];
    for (int i = 1; i < size; i++) {
        m[i] = (s[i] < m[i-1]) ? s[i] : m[i-1];
    }
}

// Calculate s_m value
void calculate_s_m(const int* s, const int* m, int* s_m, int size) {
    #pragma omp parallel for
    for (int i = 0; i < size; i++) {
        s_m[i] = s[i] - (i > 0 ? m[i-1] : 0);
    }
}

// Calculate indx
void calculate_indx(const int* s, const int* m, int* indx, int size) {
    #pragma omp parallel for
    for (int j = 0; j < size; j++) {
        indx[j] = -1;
        #pragma omp parallel for
        for (int i = 0; i <= j; i++) {
            if (s[i] == m[j]) {
                if(i==indx[j]&&indx[j]>i){
                    indx[j] = i;
                }else if(i!=indx[j]){
                    indx[j] = i;
                }
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
    int size = 300000;  // Array size
    int num_threads = 16;  // Thread number
    
    omp_set_num_threads(num_threads);




    // Test with example array first
    int ex_size = 10;
    int ex_array[] = {-2, 1, 3, -7, 11, -2, -6, 12, -3, -1};

    int* ex_s = (int*)calloc(ex_size, sizeof(int));
    int* ex_m = (int*)calloc(ex_size, sizeof(int));
    int* ex_s_m = (int*)calloc(ex_size, sizeof(int));
    int* ex_indx = (int*)calloc(ex_size, sizeof(int));

    // Calculate all arrays
    calculate_s(ex_array, ex_s, ex_size);
    calculate_m(ex_s, ex_m, ex_size);
    calculate_s_m(ex_s, ex_m, ex_s_m, ex_size);
    calculate_indx(ex_s, ex_m, ex_indx, ex_size);

    // Find MCS
    int ex_mcs_value, ex_mcs_end_index;
    find_max_value_and_index(ex_s_m, ex_size, &ex_mcs_value, &ex_mcs_end_index);

    int ex_max_indx = -1;
    for (int i = 0; i <= ex_mcs_end_index; i++) {
        if (ex_indx[i] > ex_max_indx) {
            ex_max_indx = ex_indx[i];
        }
    }

     // Debug prints to verify calculations
    printf("Information for Example Array:\n");
    printf("Original array (x): ");
    for (int i = 0; i < ex_size; i++) {
        printf("%d ", ex_array[i]);
    }
    printf("\nPrefix sums (s): ");
    for (int i = 0; i < ex_size; i++) {
        printf("%d ", ex_s[i]);
    }
    printf("\nPrefix mins (m): ");
    for (int i = 0; i < ex_size; i++) {
        printf("%d ", ex_m[i]);
    }
    printf("\nModified sums (s_m): ");
    for (int i = 0; i < ex_size; i++) {
        printf("%d ", ex_s_m[i]);
    }
    printf("\nIndex (indx): ");
    for (int i = 0; i < ex_size; i++) {
        printf("%d ", ex_indx[i]);
    }
    printf("\nExample array MCS Value: %d\n", ex_mcs_value);
    printf("Example array MCS Range: [%d, %d]\n", ex_max_indx + 1, ex_mcs_end_index);
    printf("\n\n");

    free(ex_s);
    free(ex_m);
    free(ex_s_m);
    free(ex_indx);




    
    int* x = (int*)malloc(size * sizeof(int));
    int* s = (int*)malloc(size * sizeof(int));
    int* m = (int*)malloc(size * sizeof(int));
    int* s_m = (int*)malloc(size * sizeof(int));
    int* indx = (int*)malloc(size * sizeof(int));
    

    // Generate random input array
    srand(time(NULL)); //set random seed to current time
    generate_random_array(x, size);

    double start_time = omp_get_wtime();
    
    calculate_s(x, s, size);
    calculate_m(s, m, size);
    calculate_s_m(s, m, s_m, size);
    calculate_indx(s, m, indx, size);

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


    printf("For array with size: %d and random values between %d and %d:\n", size,size,-size);    
    printf("MCS Value: %d\n", mcs_value);
    printf("MCS Range: [%d, %d]\n", max_indx + 1, mcs_end_index);
    printf("Execution Time: %f seconds with %d threads\n", end_time - start_time,num_threads);
    
    free(x);
    free(s);
    free(m);
    free(s_m);
    free(indx);
    
    return 0;
}
