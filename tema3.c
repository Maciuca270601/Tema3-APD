#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    
    int numtasks, rank;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //generate an array with specified dimension
    char* dim = argv[1];
    int dimension = atoi(dim);
    int array[dimension];
    int sizes[numtasks];

    if(rank == 0) {
        MPI_Status status;
        //open cluster file with same name as the rank of the process
        char filename[20];
        sprintf(filename, "cluster%d.txt", rank);
        FILE *fp = fopen(filename, "r");
        if(fp == NULL) {
            printf("Error opening file %s!\n", filename);
            exit(1);
        }

        int number_of_connections;
        fscanf(fp, "%d", &number_of_connections);
        int connections[4][numtasks];

        connections[0][0] = number_of_connections;
        for (int i = 1; i <= number_of_connections; i++) {
            fscanf(fp, "%d", &connections[0][i]);
        }
        fclose(fp);

        //send data to next process "->"
        MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, 3, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 3);

        //receive data from previous process "<-"
        MPI_Recv(&(connections[0][0]), (4 * numtasks), MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);

        //Print topology
        printf("%d -> ", rank);
        for(int i = 0; i < 4; i++) {
            printf("%d:", i);
            for(int j = 1; j < connections[i][0]; j++) {
                printf("%d,", connections[i][j]);
            }
            printf("%d ", connections[i][connections[i][0]]);
        }
        printf("\n");

        //send data to workers
        for(int w = 1; w <= number_of_connections; w++) {
            MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, connections[0][w], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, connections[0][w]);
        }

        //generate array
        for(int k = 0; k < dimension; k++) {
            array[k] = dimension - k - 1;
        }

        //send array to rank 3
        MPI_Send(&array, dimension, MPI_INT, 3, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 3);

        int size = (dimension / (numtasks - 4));
        int rest = dimension - size * (numtasks - 4);

        //complete sizes array for rank 0
        for(int i = 0; i < 4; i++) {
            for(int w = 1; w <= connections[i][0]; w++) {
                if (rest - 1 >= 0) {
                    rest--;
                    sizes[connections[i][w]] = size + 1;
                } else {
                    sizes[connections[i][w]] = size;
                }
            }
        }

        //print sizes array 
        printf("SIZES ARE: ");
        for(int i = 0; i < numtasks; i++) {
            printf("%d ", sizes[i]);
        }
        printf("\n");

        int offset = 0;
        int copy_offset = offset;
        //send array to workers
        for(int w = 1; w <= number_of_connections; w++) {
            int size =  sizes[connections[0][w]];

            MPI_Send(&(array[offset]), size, MPI_INT, connections[0][w], 0, MPI_COMM_WORLD);
            offset += size;
            printf("M(%d,%d)\n", rank, connections[0][w]);
        }

        //receive array from workers
        MPI_Status status2;
        for(int w = 1; w <= number_of_connections; w++) {
            int size = sizes[connections[0][w]];

            MPI_Recv(&(array[copy_offset]), size, MPI_INT, connections[0][w], 0, MPI_COMM_WORLD, &status2);
            copy_offset += size;
            printf("M(%d,%d)\n", status2.MPI_SOURCE, rank);
        }

        //receive array from rank 3
        int tmp_array[dimension];
        MPI_Status status3;
        MPI_Recv(&tmp_array, dimension, MPI_INT, 3, 0, MPI_COMM_WORLD, &status3);
        printf("M(%d,%d)\n", status3.MPI_SOURCE, rank);

        //merge array
        offset = 0;
        for(int i = 1; i <= connections[0][0]; i++) {
            offset += sizes[connections[0][i]];
        }
        //copy connection[3][0];
        for(int i = 1; i <= connections[3][0]; i++) {
            for(int j = 0; j < sizes[connections[3][i]]; j++) {
                array[offset] = tmp_array[offset];
                offset++;
            }
        }
        //copy connection[2][0];
        for(int i = 1; i <= connections[2][0]; i++) {
            for(int j = 0; j < sizes[connections[2][i]]; j++) {
                array[offset] = tmp_array[offset];
                offset++;
            }
        }
        //copy connection[1][0]
        for(int i = 1; i <= connections[1][0]; i++) {
            for(int j = 0; j < sizes[connections[1][i]]; j++) {
                array[offset] = tmp_array[offset];
                offset++;
            }
        }

        //print whole array
        printf("Rezultat: ");
        for(int i = 0; i < dimension; i++) {
            printf("%d ", array[i]);
        }
        printf("\n");


    } else if (rank == 1) {
        MPI_Status status;
        char filename[20];
        sprintf(filename, "cluster%d.txt", rank);
        FILE *fp = fopen(filename, "r");
        if(fp == NULL) {
            printf("Error opening file %s!\n", filename);
            exit(1);
        }

        int connections[4][numtasks];
        //receive previous data "->"
        MPI_Recv(&(connections[0][0]), (4 * numtasks), MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);
        int number_of_connections;
        fscanf(fp, "%d", &number_of_connections);
        connections[1][0] = number_of_connections;
        for (int i = 1; i <= number_of_connections; i++) {
            fscanf(fp, "%d", &connections[1][i]);
        }
        fclose(fp);

        //Print topology
        printf("%d -> ", rank);
        for(int i = 0; i < 4; i++) {
            printf("%d:", i);
            for(int j = 1; j < connections[i][0]; j++) {
                printf("%d,", connections[i][j]);
            }
            printf("%d ", connections[i][connections[i][0]]);
        }
        printf("\n");

        //send new data "<-"
        MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);

        //send data to workers
        for(int w = 1; w <= number_of_connections; w++) {
            MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, connections[1][w], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, connections[1][w]);
        }

        int array[dimension];
        //receive array from rank 2
        MPI_Recv(&array, dimension, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);

        int size = (dimension / (numtasks - 4));
        int rest = dimension - size * (numtasks - 4);

        //complete sizes array for rank 0
        for(int i = 0; i < 4; i++) {
            for(int w = 1; w <= connections[i][0]; w++) {
                if (rest - 1 >= 0) {
                    rest--;
                    sizes[connections[i][w]] = size + 1;
                } else {
                    sizes[connections[i][w]] = size;
                }
            }
        }

        //send array to workers
        int offset = dimension;
        for(int i = 1; i <= connections[1][0]; i++) {
            offset -= sizes[connections[1][i]];
        }
        int copy_offset = offset;

        for(int w = 1; w <= number_of_connections; w++) {

            int size = sizes[connections[1][w]];

            MPI_Send(&(array[offset]), size, MPI_INT, connections[1][w], 0, MPI_COMM_WORLD);
            offset += size;
            printf("M(%d,%d)\n", rank, connections[1][w]);
        }

        //receive array from workers
        MPI_Status status2;
        for(int w = 1; w <= number_of_connections; w++) {
            int size = sizes[connections[1][w]];
            
            MPI_Recv(&(array[copy_offset]), size, MPI_INT, connections[1][w], 0, MPI_COMM_WORLD, &status2);
            copy_offset += size;
            printf("M(%d,%d)\n", status2.MPI_SOURCE, rank);
        }

        //send array to rank 2
        MPI_Send(&array, dimension, MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);

    } else if (rank == 2) {
        MPI_Status status;
        char filename[20];
        sprintf(filename, "cluster%d.txt", rank);
        FILE *fp = fopen(filename, "r");
        if(fp == NULL) {
            printf("Error opening file %s!\n", filename);
            exit(1);
        }

        int connections[4][numtasks];
        //receive previous data "->"
        MPI_Recv(&(connections[0][0]), (4 * numtasks), MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);
        int number_of_connections;
        fscanf(fp, "%d", &number_of_connections);
        connections[2][0] = number_of_connections;
        for (int i = 1; i <= number_of_connections; i++) {
            fscanf(fp, "%d", &connections[2][i]);
        }
        fclose(fp);

        //send new data "->"
        MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 1);
        
        //receive new data "<-"
        MPI_Recv(&(connections[0][0]), (4 * numtasks), MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);

        //Print topology
        printf("%d -> ", rank);
        for(int i = 0; i < 4; i++) {
            printf("%d:", i);
            for(int j = 1; j < connections[i][0]; j++) {
                printf("%d,", connections[i][j]);
            }
            printf("%d ", connections[i][connections[i][0]]);
        }
        printf("\n");

        //send new data "<-"
        MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, 3, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 3);

        //send data to workers
        for(int w = 1; w <= number_of_connections; w++) {
            MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, connections[2][w], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, connections[2][w]);
        }

        //receive array from rank 3
        int array[dimension];
        MPI_Recv(&array, dimension, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);

        //send array to rank 1
        MPI_Send(&array, dimension, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 1);

        int size = (dimension / (numtasks - 4));
        int rest = dimension - size * (numtasks - 4);

        //complete sizes array for rank 0
        for(int i = 0; i < 4; i++) {
            for(int w = 1; w <= connections[i][0]; w++) {
                if (rest - 1 >= 0) {
                    rest--;
                    sizes[connections[i][w]] = size + 1;
                } else {
                    sizes[connections[i][w]] = size;
                }
            }
        }

        int offset = 0;
        int copy_offset = 0;
        for(int i = 1; i <= connections[0][0]; i++) {
            offset += sizes[connections[0][i]];
        }
        for(int i = 1; i <= connections[3][0]; i++) {
            offset += sizes[connections[3][i]];
        }
        copy_offset = offset;

        //send array to workers
        for(int w = 1; w <= number_of_connections; w++) {

            int size = sizes[connections[2][w]];

            MPI_Send(&(array[offset]), size, MPI_INT, connections[2][w], 0, MPI_COMM_WORLD);
            offset += size;
            printf("M(%d,%d)\n", rank, connections[2][w]);
        }

        //receive array from workers
        MPI_Status status2;
        for(int w = 1; w <= number_of_connections; w++) {
            int size = sizes[connections[2][w]];

            MPI_Recv(&(array[copy_offset]), size, MPI_INT, connections[2][w], 0, MPI_COMM_WORLD, &status2);
            copy_offset += size;
            printf("M(%d,%d)\n", status2.MPI_SOURCE, rank);
        }

        //receive array from rank 1
        int tmp_array[dimension];
        MPI_Status status3;
        MPI_Recv(&tmp_array, dimension, MPI_INT, 1, 0, MPI_COMM_WORLD, &status3);
        printf("M(%d,%d)\n", status3.MPI_SOURCE, rank);

        //merge array
        offset = dimension;
        //get offset from dimension - connections[1][0];
        for(int i = 1; i <= connections[1][0]; i++) {
            offset -= sizes[connections[1][i]];
        }

        //copy connections[1][0];
        for(int i = 1; i <= connections[1][0]; i++) {
            for(int j = 0; j < sizes[connections[1][i]]; j++) {
                array[offset] = tmp_array[offset];
                offset++;
            }
        }

        //send array to rank 3
        MPI_Send(&array, dimension, MPI_INT, 3, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 3);

    } else if (rank == 3) {
        MPI_Status status;
        char filename[20];
        sprintf(filename, "cluster%d.txt", rank);
        FILE *fp = fopen(filename, "r");
        if(fp == NULL) {
            printf("Error opening file %s!\n", filename);
            exit(1);
        }

        int connections[4][numtasks];
        //receive previous data "->"
        MPI_Recv(&(connections[0][0]), (4 * numtasks), MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);
        int number_of_connections;
        fscanf(fp, "%d", &number_of_connections);

        connections[3][0] = number_of_connections;
        for (int i = 1; i <= number_of_connections; i++) {
            fscanf(fp, "%d", &connections[3][i]);
        }
        fclose(fp);

        //send new data "->"
        MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);

        //receive new data "<-"
        MPI_Recv(&(connections[0][0]), (4 * numtasks), MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);
        
        //Print topology
        printf("%d -> ", rank);
        for(int i = 0; i < 4; i++) {
            printf("%d:", i);
            for(int j = 1; j < connections[i][0]; j++) {
                printf("%d,", connections[i][j]);
            }
            printf("%d ", connections[i][connections[i][0]]);
        }
        printf("\n");

        //send new data "<-"
        MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, 0, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 0);

        //send data to workers
        for(int w = 1; w <= number_of_connections; w++) {
            MPI_Send(&(connections[0][0]), (4 * numtasks), MPI_INT, connections[3][w], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, connections[3][w]);
        }

        int array[dimension];
        //receive array from rank 0
        MPI_Recv(&array, dimension, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);

        //send array to rank 2
        MPI_Send(&array, dimension, MPI_INT, 2, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 2);

        int size = (dimension / (numtasks - 4));
        int rest = dimension - size * (numtasks - 4);

        //complete sizes array for rank 0
        for(int i = 0; i < 4; i++) {
            for(int w = 1; w <= connections[i][0]; w++) {
                if (rest - 1 >= 0) {
                    rest--;
                    sizes[connections[i][w]] = size + 1;
                } else {
                    sizes[connections[i][w]] = size;
                }
            }
        }

        int offset = 0;
        int copy_offset = 0;
        for(int i = 1; i <= connections[0][0]; i++) {
            offset += sizes[connections[0][i]];
        }
        copy_offset = offset;

        //send array to workers
        for(int w = 1; w <= number_of_connections; w++) {

            int size = sizes[connections[3][w]];

            MPI_Send(&(array[offset]), size, MPI_INT, connections[3][w], 0, MPI_COMM_WORLD);
            offset += size;
            printf("M(%d,%d)\n", rank, connections[3][w]);
        }

        //receive array from workers
        MPI_Status status2;
        for(int w = 1; w <= number_of_connections; w++) {
            int size = sizes[connections[3][w]];

            MPI_Recv(&(array[copy_offset]), size, MPI_INT, connections[3][w], 0, MPI_COMM_WORLD, &status2);
            copy_offset += size;
            printf("M(%d,%d)\n", status2.MPI_SOURCE, rank);
        }

        //receive array from rank 2
        int tmp_array[dimension];
        MPI_Status status3;
        MPI_Recv(&tmp_array, dimension, MPI_INT, 2, 0, MPI_COMM_WORLD, &status3);
        printf("M(%d,%d)\n", status3.MPI_SOURCE, rank);

        //merge array
        offset = 0;
        //get offset
        for(int i = 1; i <= connections[0][0]; i++) {
            offset += sizes[connections[0][i]];
        }
        for(int i = 1; i <= connections[3][0]; i++) {
            offset += sizes[connections[3][i]];
        }

        //copy connections[1][0] + connections[2][0];
        for(int i = 1; i <= connections[1][0]; i++) {
            for(int j = 0; j < sizes[connections[1][i]]; j++) {
                array[offset] = tmp_array[offset];
                offset++;
            }
        }
        for(int i = 1; i <= connections[2][0]; i++) {
            for(int j = 0; j < sizes[connections[2][i]]; j++) {
                array[offset] = tmp_array[offset];
                offset++;
            }
        }

        //send array to rank 0
        MPI_Send(&array, dimension, MPI_INT, 0, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, 0);

    } else {
        //Worker
        MPI_Status status;
        int connections[4][numtasks];
        MPI_Recv(&(connections[0][0]), (4 * numtasks), MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        printf("M(%d,%d)\n", status.MPI_SOURCE, rank);

        //Print topology
        printf("%d -> ", rank);
        for(int i = 0; i < 4; i++) {
            printf("%d:", i);
            for(int j = 1; j < connections[i][0]; j++) {
                printf("%d,", connections[i][j]);
            }
            printf("%d ", connections[i][connections[i][0]]);
        }
        printf("\n");

        int size = (dimension / (numtasks - 4));
        int rest = dimension - size * (numtasks - 4);

        //complete sizes array for rank 0
        for(int i = 0; i < 4; i++) {
            for(int w = 1; w <= connections[i][0]; w++) {
                if (rest - 1 >= 0) {
                    rest--;
                    sizes[connections[i][w]] = size + 1;
                } else {
                    sizes[connections[i][w]] = size;
                }
            }
        }

        //receive array from clusters
        MPI_Status status2;
        int array[dimension];
        int lungime = sizes[rank];
        MPI_Recv(&array, lungime, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status2);
        printf("M(%d,%d)\n", status2.MPI_SOURCE, rank);

        //multiply array values with 5
        for(int i = 0; i < lungime; i++) {
            array[i] = array[i] * 5;
        }

        //send array to clusters
        MPI_Send(&array, lungime, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
        
    }
    





    MPI_Finalize();
    return 0;
}