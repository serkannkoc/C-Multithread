#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <ctype.h>

// prototypes
void createThreads(int READ_THREADS, int UPPER_THREADS, int REPLACE_THREADS, int WRITE_THREADS, const pthread_t *readThreads,
              const pthread_t *upperThreads, const pthread_t *replaceThreads, const pthread_t *writeThreads);

void joinThreads(int READ_THREADS, int UPPER_THREADS, int REPLACE_THREADS, int WRITE_THREADS, const pthread_t *readThreads,
            const pthread_t *upperThreads, const pthread_t *replaceThreads, const pthread_t *writeThreads);

void calculateNumberOfLinesInFile(char *const *argv);

void *readFile(void *args);

void *upper(void *args);

void *replace(void *args);

void *writeFile(void *args);

// constants
#define MAX_LINE_LENGTH 256
#define TOTAL_LINES 100000
// Threads
pthread_mutex_t mutexLineBuffer;
pthread_mutex_t mutexFile;
// Semaphores
sem_t semEmpty1;
sem_t semFull1;
sem_t semEmpty2;
sem_t semFull2;

// Global Variables
int numberOfLinesInFile;
int index1 = 0;
int index2 = 0;

struct line_struct {
    char line[MAX_LINE_LENGTH];
    int lineNumber;
};
struct line_struct line_buffer[TOTAL_LINES];



char fileName[50];
int countReadLine = 0;
int countUpperedLine = 0;
int countWrittenLine = 0;




int main(int argc,char *argv[]){

    //check for errors
    char* errorMessage = "ERROR: Usage: ./project3.o -d filename.txt -n #ReadThreads #UpperThreads #ReplaceThreads #WriteThreads\n";
    if (argc != 8){
        fprintf(stderr, "%s", errorMessage);
        return 1;
    }
    else if (strcmp(argv[1], "-d") != 0){
        fprintf(stderr, "%s", errorMessage);
        return 1;
    }
    else if (access(argv[2], F_OK) == -1){
        fprintf(stderr, "ERROR: File does not exists !\n");
        return 1;
    }
    else if (strcmp(argv[3], "-n") != 0){
        fprintf(stderr, "%s", errorMessage);
        return 1;
    }

    // define threads
    int READ_THREADS = atoi(argv[4]);
    int UPPER_THREADS = atoi(argv[5]);
    int REPLACE_THREADS = atoi(argv[6]);
    int WRITE_THREADS = atoi(argv[7]);

    pthread_t readThreads[READ_THREADS];
    pthread_t upperThreads[UPPER_THREADS];
    pthread_t replaceThreads[REPLACE_THREADS];
    pthread_t writeThreads[WRITE_THREADS];



    //open file and calculate the number of the lines in the txt
    calculateNumberOfLinesInFile(argv);

    //initialize the global file name
    strcpy(fileName, argv[2]);

    //mutex initialization
    pthread_mutex_init(&mutexLineBuffer,NULL);
    pthread_mutex_init(&mutexFile,NULL);


    //semaphore initialization
    sem_init(&semEmpty1, 0, numberOfLinesInFile);
    sem_init(&semFull1, 0, 0);
    sem_init(&semEmpty2, 0, numberOfLinesInFile);
    sem_init(&semFull2, 0, 0);

    // creating threads
    createThreads(READ_THREADS, UPPER_THREADS, REPLACE_THREADS, WRITE_THREADS, readThreads, upperThreads,
                  replaceThreads,
                  writeThreads);

    // joining threads
    joinThreads(READ_THREADS, UPPER_THREADS, REPLACE_THREADS, WRITE_THREADS, readThreads, upperThreads, replaceThreads,
                writeThreads);

    // destroying
    sem_destroy(&semEmpty1);
    sem_destroy(&semFull1);
    sem_destroy(&semEmpty2);
    sem_destroy(&semFull2);

    pthread_mutex_destroy(&mutexLineBuffer);
    pthread_mutex_destroy(&mutexFile);


}
// Thread Functions
void *readFile(void *args){
    long tid;
    tid = (long)args;
    while (1){
        // Produce
        FILE *file = fopen(fileName,"r");
        if (file == NULL) {
            perror("Error opening file");
            return (NULL);
        }
//        printf("%d",countReadLine);
        char line[MAX_LINE_LENGTH];
        sleep(4);
        sem_wait(&semEmpty1);
        pthread_mutex_lock(&mutexLineBuffer);
        fseek(file, index1, SEEK_SET);
        if (fgets(line, sizeof(line), file) != NULL) {
//            printf("%s" ,line);
            strcpy(line_buffer[countReadLine].line,line);
            line_buffer[countReadLine].lineNumber = countReadLine;
            printf("--R%ld--%s\n",tid, line_buffer[countReadLine].line);
            countReadLine++;
//            printf("%s", line_buffer->line);

//            sleep(4);
        }
        index1 += strlen(line);
        pthread_mutex_unlock(&mutexLineBuffer);
        sem_post(&semFull1);
        fclose(file);

    }

}

void *upper(void *args){
    long tid;
    tid = (long)args;
    while (1) {
        sem_wait(&semFull1);
        pthread_mutex_lock(&mutexLineBuffer);

//          printf("Upper_%ld Upper_%ld read index %d and converted “%s”", tid, tid, lineCountUpperCase, lines_struct[lineCountUpperCase].line_str);
//            printf("U%ld ",tid);
            for (int j = 0; line_buffer[countUpperedLine].line[j] != '\0'; j++) {
                line_buffer[countUpperedLine].line[j] = toupper((unsigned char)line_buffer[countUpperedLine].line[j]);
            }
            printf("--U%ld--%s\n",tid, line_buffer[countUpperedLine].line);

//                printf(" to “%s”\n", lines_struct[lineCountUpperCase].line_str);
            countUpperedLine++;

        pthread_mutex_unlock(&mutexLineBuffer);

        // write için sem post eklencek sonuna
        sem_post(&semFull2);
    }
}
void *replace(void *args){

}
void *writeFile(void *args){
    long tid;
    tid = (long)args;
    while (1){
        sem_wait(&semFull2);
        pthread_mutex_lock(&mutexFile);
        FILE *fp;
        fp = fopen(fileName, "r+");
        fseek(fp, index2, SEEK_SET);
        fputs(line_buffer[countWrittenLine].line, fp);
        fflush(fp);
        index2 += strlen(line_buffer[countWrittenLine].line);
        fclose(fp);
//      printf("Writer_%ld Writer_%ld write line %d back which is “%s”\n", tid, tid, lineCountWrite, lines_struct[lineCountWrite].line_str);
        printf("--W%ld--%s\n",tid, line_buffer[countWrittenLine].line);
        countWrittenLine++;
        pthread_mutex_unlock(&mutexFile);
    }
}
void calculateNumberOfLinesInFile(char *const *argv) {
    FILE *fp = fopen(argv[2], "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        exit(EXIT_FAILURE);
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        numberOfLinesInFile++;
    }
    fclose(fp);
}

void joinThreads(int READ_THREADS, int UPPER_THREADS, int REPLACE_THREADS, int WRITE_THREADS, const pthread_t *readThreads,
            const pthread_t *upperThreads, const pthread_t *replaceThreads, const pthread_t *writeThreads) {
    long i;
    for (i = 0; i < READ_THREADS; i++)
        pthread_join(readThreads[i], NULL);

    for (i = 0; i < UPPER_THREADS; i++)
        pthread_join(upperThreads[i], NULL);

//    for (i = 0; i < REPLACE_THREADS; i++)
//        pthread_join(replaceThreads[i], NULL);
//
    for (i = 0; i < WRITE_THREADS; i++)
        pthread_join(writeThreads[i], NULL);
}

void createThreads(int READ_THREADS, int UPPER_THREADS, int REPLACE_THREADS, int WRITE_THREADS, const pthread_t *readThreads,
              const pthread_t *upperThreads, const pthread_t *replaceThreads, const pthread_t *writeThreads) {
    long i;
    int rc;

    for (i = 0; i < READ_THREADS; i++){
        rc = pthread_create(&readThreads[i], NULL, &readFile, (void *)i);
        printf("Read Thread %d is created\n", i);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    for (i = 0; i < UPPER_THREADS; i++){
        rc = pthread_create(&upperThreads[i], NULL, &upper, (void *)i);
        printf("Upper Thread %d is created\n", i);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
//    for (i = 0; i < REPLACE_THREADS; i++){ /* creating replace threads */
//        rc = pthread_create(&replaceThreads[i], NULL, &replace, (void *)i);
//        printf("Replace Thread %d is created\n", i);
//        if (rc){
//            printf("ERROR; return code from pthread_create() is %d\n", rc);
//            exit(-1);
//        }
//    }
    for (i = 0; i < WRITE_THREADS; i++){ /* creating write threads */
        rc = pthread_create(&writeThreads[i], NULL, &writeFile, (void *)i);
        printf("Write Thread %d is created\n", i);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
}
