#include <stdio.h>
#define SIZE 2

void inputMatrix(int mat[SIZE][SIZE], const char *name) {
    printf("Enter elements of %s matrix:\n", name);
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            scanf("%d", &mat[i][j]);
        }
    }
}

void multiplyMatrices(int mat1[SIZE][SIZE], int mat2[SIZE][SIZE], int result[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            result[i][j] = 0;
            for (int k = 0; k < SIZE; k++) {
                result[i][j] += mat1[i][k] * mat2[k][j];
            }
        }
    }
}

void displayMatrix(int mat[SIZE][SIZE]) {
    printf("Resultant matrix:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

int main() {
    int a[SIZE][SIZE], b[SIZE][SIZE], result[SIZE][SIZE];

    inputMatrix(a, "first");
    inputMatrix(b, "second");
    multiplyMatrices(a, b, result);
    displayMatrix(result);

    return 0;
}