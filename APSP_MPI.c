#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

int  n, edge_num; //vertice數
int *final_graph, *graph, *n_index , *edge;
const int INFINITY = 999;


void read_file(char *);
void write_file(char *);
void split(char **, char *);
char *int_to_string(int num, char *str);



int main(int argc, char* argv[]) {

    assert(argc == 4);

    int i, j;
    int procs_num, rank;
    procs_num = atoi(argv[3]);

    read_file(argv[1]);
    
    MPI_Comm Comm_graph;
    MPI_Init(&argc, &argv);
    MPI_Graph_create(MPI_COMM_WORLD, n, n_index, edge, 1, &Comm_graph);
    MPI_Comm_rank(Comm_graph, &rank);
    

    int done = 1, neigh_num;
    MPI_Graph_neighbors_count(Comm_graph, rank, &neigh_num); // 相鄰的node數

    int send_buf[n], recv_buf[ n * neigh_num ] , neighbers[neigh_num];
    MPI_Graph_neighbors(Comm_graph, rank, neigh_num, neighbers); // 相鄰有哪些node

    for( i = 0; i < n ; i++) send_buf[i] = graph[rank * n + i]; // Initialize

    while(done) { // 1 == true
        done = 0;
        MPI_Neighbor_allgather(send_buf, n, MPI_INT, recv_buf, n, MPI_INT, Comm_graph);
        
        for( i = 0; i < neigh_num; i++) {

            int k = neighbers[i];

            for( j = 0; j < n; j++) {
                if(j != rank) {
                    if(send_buf[j] > send_buf[k] + recv_buf[i*n + j]) {
                        send_buf[j] = send_buf[k] + recv_buf[i*n + j];
                        done = 1;
                    }
                }
            }
        }

        int tmp = done;
        MPI_Allreduce(&tmp, &done, 1, MPI_INT, MPI_BOR, Comm_graph); // Bit-wise OR each process的relax只要一個是true就繼續做

    }

    
    if(rank == 0) 
        final_graph = malloc(n * n * sizeof(int));
    MPI_Gather(send_buf, n, MPI_INT, final_graph, n, MPI_INT, 0, Comm_graph);
    if(rank == 0) 
        write_file(argv[2]);
    
    MPI_Finalize();
    return 0;
}  

void read_file(char *filename)
{
    const int LINE_LENGTH = 64;
    char line[64];
    char *arr[3];
    int i, j;

    FILE *input_file = fopen(filename, "r");
    if (!input_file)
    {
        puts("Error occurs while opening the input file");
        exit(-1);
    }
    //只先讀取第一行 line:存入的陣列 LINE_LENGTH:該行最多幾個字元
    fgets(line, LINE_LENGTH, input_file); 
    split(arr, line);
    n = atoi(arr[0]); // Number of vertex
    edge_num = n*(n-1)/2;

    //Initialize Matrix
    graph = malloc(n * n * sizeof(int));

    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            graph[i * n + j] = (i == j) ? 0 : INFINITY;


    while (fgets(line, LINE_LENGTH, input_file)) //一行行讀取
    {
        split(arr, line);
        int x = atoi(arr[0]);
        int y = atoi(arr[1]);
        int value = atoi(arr[2]);

        // Since the input graph is an undirect graph
        graph[x * n + y] = graph[y * n + x] = value;
    }

    n_index = malloc(n * sizeof(int));
    edge = malloc( 2 * edge_num * sizeof(int));
    int ind = 0, edg = 0;

    for(i = 0; i < n; i++) {
        for(j = 0; j < n; j++) {
            if(i!=j && graph[i * n + j] != INFINITY) {
                ind++;  
                edge[edg++] = j;
            }
        }
        n_index[i] = ind;
    }

    fclose(input_file);
}


void write_file(char *filename)
{
    FILE *output_file = fopen(filename, "w");
    if (!output_file)
    {
        puts("Error occurs while opening the output file");
        exit(-1);
    }

    int i, j;
    char tmp[16];

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            fputs(int_to_string(final_graph[i * n + j], tmp), output_file);
            fputs(" ", output_file);
        }
        fputs("\n", output_file);
    }
    fclose(output_file);
}


char *int_to_string(int num, char *str)
{
    if (str == NULL)
        return NULL;

    sprintf(str, "%d", num);
    return str;
}


void split(char **arr, char *str)
{
    char *s = strtok(str, " "); //以第二個參數字串的內容切割第一個參數字串
    while (s != NULL)
    {
        *arr++ = s;
        s = strtok(NULL, " ");
    }
}
