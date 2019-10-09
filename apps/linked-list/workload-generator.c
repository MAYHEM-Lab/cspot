#include <time.h>
#include <stdio.h>

void reader_test(char *fname){

    FILE *fp;
    int num_of_operations;
    int i;
    int op;
    int val;

    fp = fopen(fname, "r");
    fscanf(fp, "%d", &num_of_operations);
    printf("%d\n", num_of_operations);

    for(i = 0; i < num_of_operations; ++i){
        fscanf(fp, "%d", &op);
        fscanf(fp, "%d", &val);
        printf("%d %d\n", op, val);
    }

    fclose(fp);

}

int main(int argc, char *argv[]){

    int num_of_operations;
    int min_val;
    int max_val;
    int i;
    int op;
    int val;
    char *fname = "workload.txt";

    FILE *fp;

    num_of_operations = 1000;
    min_val = 1;
    max_val = 100;

    fp = fopen(fname, "w");
    fprintf(fp, "%d\n", num_of_operations);

    srand(time(NULL));
    for(i = 0; i < num_of_operations; ++i){
        op = rand() % 2;
        val = min_val + rand() % (max_val - min_val + 1);
        fprintf(fp, "%d %d\n", op, val);
    }

    fclose(fp);

    reader_test(fname);

    return 0;

}
