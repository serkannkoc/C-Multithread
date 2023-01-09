#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <ctype.h>


#define MAX_LINE_LENGTH 200
#define NUMBER_OF_LINES 100000


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
sem_t semUpper;
sem_t semReplace;
sem_t semWrite1;
sem_t semWrite2;
sem_t fullCount;


struct lines_str {
    char line_str[MAX_LINE_LENGTH];
    int readed;
    int isUpper;
    int isUnderscore;
    int isWritten;
};

struct lines_str lines_struct[NUMBER_OF_LINES];

char filename[50];
int index1 = 0;
int fileIndex2 = 0;
int lineCountRead = 0;
int lineCountUpperCase = 0;
int lineCountReplace = 0;
int lineCountWrite = 0;
int lineEnd = 0;
int fileEnd = 0;

void *readFile(void *threadId) {
    long tid;
    tid = (long)threadId;

    while (1) {
        FILE *file = fopen(filename, "r");
        if (index1 > fileEnd) {
            return (NULL);
        }
        if (file != NULL && feof(file) != EOF) {
            char line[MAX_LINE_LENGTH];
//            sem_wait(&semWrite2); //lock the semaphore
            pthread_mutex_lock(&lock);
            fseek(file, index1, SEEK_SET);
            if (fgets(line, sizeof(line), file) != NULL) {
                strcpy(lines_struct[lineCountRead].line_str, line);
                lines_struct[lineCountRead].readed = 1;
//                printf("Read_%ld	  	Read_%ld read the line %d which is \"%s\"\n", tid, tid, lineCountRead, lines_struct[lineCountRead].line_str);
                printf("R%ld ",tid);
                lineCountRead++;
            }
            index1 += strlen(line);
            pthread_mutex_unlock(&lock);
            sem_post(&semUpper);
            sem_post(&semReplace);
            fclose(file);
        }
        else{
            perror(filename);
        }
    }
}

void *repToUpper(void *threadId) {
    long tid;
    tid = (long)threadId;

    int  j;
    while (1) {
        if (lineCountUpperCase + 1 >= lineEnd)
            return (NULL);
        if (lines_struct[lineCountUpperCase].line_str != "" && lines_struct[lineCountUpperCase].isUpper != 1) {
            sem_wait(&semUpper);
            pthread_mutex_lock(&lock);
            if(lineCountUpperCase < lineCountRead){
//                printf("Upper_%ld	  	Upper_%ld read index %d and converted “%s”", tid, tid, lineCountUpperCase, lines_struct[lineCountUpperCase].line_str);
                printf("U%ld ",tid);
                for (j = 0; lines_struct[lineCountUpperCase].line_str[j] != '\0'; j++) {
                    lines_struct[lineCountUpperCase].line_str[j] = toupper((unsigned char)lines_struct[lineCountUpperCase].line_str[j]);
                }
                lines_struct[lineCountUpperCase].isUpper = 1;
//                printf(" to “%s”\n", lines_struct[lineCountUpperCase].line_str);
                lineCountUpperCase++;
            }
            pthread_mutex_unlock(&lock);
            sem_post(&semWrite1);
        }
    }
}

//void *repToUnderscore(void *threadId){
//    long tid;
//    tid = (long)threadId;
//    int j;
//    while (1) {
//        if (lineCountReplace + 1 >= lineEnd)
//            return (NULL);
//        if (lines_struct[lineCountReplace].line_str != "" && lines_struct[lineCountReplace].isUnderscore != 1) {
//            sem_wait(&semReplace);
//            pthread_mutex_lock(&lock);
//            if(lineCountReplace < lineCountUpperCase){
//                printf("Replace_%ld	Replace_%ld read index %d and converted “%s”", tid, tid, lineCountReplace, lines_struct[lineCountReplace].line_str);
////                printf("_%ld ",tid);
//                for (j = 0; lines_struct[lineCountReplace].line_str[j] != '\0'; j++) {
//                    if (lines_struct[lineCountReplace].line_str[j] == ' ')
//                        lines_struct[lineCountReplace].line_str[j] = '_';
//                }
//                lines_struct[lineCountReplace].isUnderscore = 1;
//                printf(" to “%s”\n", lines_struct[lineCountReplace].line_str);
//                lineCountReplace++;
//            }
//            pthread_mutex_unlock(&lock);
//            sem_post(&semWrite2);
//        }
//    }
//}

void *writeFile(void *threadId){
    long tid;
    tid = (long)threadId;
    while (1) {
        if (lineCountWrite + 1 >= lineEnd)
            return (NULL);
        if (lines_struct[lineCountWrite].line_str != "" && lines_struct[lineCountWrite].isWritten != 1){
            sem_wait(&semWrite1);
            sem_wait(&semWrite2);
            pthread_mutex_lock(&lock);
            if (lineCountWrite < lineCountReplace){
                FILE *fp;
                fp = fopen(filename, "r+");
                fseek(fp, fileIndex2, SEEK_SET);
                fputs(lines_struct[lineCountWrite].line_str, fp);
                fflush(fp);
                fileIndex2 += strlen(lines_struct[lineCountWrite].line_str);
                fclose(fp);
                lines_struct[lineCountWrite].isWritten = 1;
//                printf("Writer_%ld	Writer_%ld write line %d back which is “%s”\n", tid, tid, lineCountWrite, lines_struct[lineCountWrite].line_str);
                printf("W%ld ",tid);
                lineCountWrite++;
            }
            pthread_mutex_unlock(&lock);
//            sem_post(&semWrite1); //unlock the semaphore
        }
    }
}

int main(int argc, char *argv[]) {
    //error control
    if (argc != 8){
        fprintf(stderr, "ERROR: Usage: ./project3.o -d filename.txt -n numOfReadThreads numOfUpperThreads numOfReplaceThreads numOfWrtiteThreads\n");
        return 1;
    }
    else if (strcmp(argv[1], "-d") != 0){
        fprintf(stderr, "ERROR: Usage: ./project3.o -d filename.txt -n numOfReadThreads numOfUpperThreads numOfReplaceThreads 		numOfWrtiteThreads\n");
        return 1;
    }
    else if (access(argv[2], F_OK) == -1){ /* check for given .txt file exists */
        fprintf(stderr, "ERROR: File does not exists !\n");
        return 1;
    }
    else if (strcmp(argv[3], "-n") != 0){
        fprintf(stderr, "ERROR: Usage: ./project3.o -d filename.txt -n numOfReadThreads numOfUpperThreads numOfReplaceThreads 	 numOfWrtiteThreads\n");
        return 1;
    }

    int NUM_READ_THREADS = atoi(argv[4]);
    int NUM_UPPER_THREADS = atoi(argv[5]);
//    int NUM_REPLACE_THREADS = atoi(argv[6]);
    int NUM_WRITE_THREADS = atoi(argv[7]);

    pthread_t readThreads[NUM_READ_THREADS];
    pthread_t upperThreads[NUM_UPPER_THREADS];
//    pthread_t replaceThreads[NUM_REPLACE_THREADS];
    pthread_t writeThreads[NUM_WRITE_THREADS];

    //initialize the values of the array
    for (int k = 0; k < NUMBER_OF_LINES; k++){
        strcpy(lines_struct[k].line_str, "");
        lines_struct[k].readed = 0;
        lines_struct[k].isUnderscore = 0;
        lines_struct[k].isUpper = 0;
        lines_struct[k].isWritten = 0;
    }

    //read filename
    strcpy(filename, argv[2]);
    FILE *file = fopen(filename, "r");
    size_t len = 0;
    ssize_t read;
    char *line = NULL;

    while ((read = getline(&line, &len, file)) != -1) {
        lineEnd++;
    }

    //calculate the length of the file
    fseek(file, index1, SEEK_END);
    fileEnd = ftell(file);

    int rc;
    long t;

    //mutex initialization
    if (pthread_mutex_init(&lock, NULL) != 0){
        printf("\n mutex init has failed\n");
        return 1;
    }

    //semaphore initialization
    sem_init(&semWrite1, 0, 0);
    sem_init(&semWrite2, 0, 0);
    sem_init(&semUpper, 0, 0);
    sem_init(&semReplace, 0, 0);

    //creating the threads
    for (t = 0; t < NUM_READ_THREADS; t++){ /* creating read threads */
        rc = pthread_create(&readThreads[t], NULL, &readFile, (void *)t);
        printf("Read Thread %d is created\n",t);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t = 0; t < NUM_UPPER_THREADS; t++){ /* creating read threads */
        rc = pthread_create(&upperThreads[t], NULL, &repToUpper, (void *)t);
        printf("Upper Thread %d is created\n",t);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

//    for (t = 0; t < NUM_REPLACE_THREADS; t++){ /* creating replace threads */
//        rc = pthread_create(&replaceThreads[t], NULL, &repToUnderscore, (void *)t);
//        printf("Replace Thread %d is created\n",t);
//        if (rc){
//            printf("ERROR; return code from pthread_create() is %d\n", rc);
//            exit(-1);
//        }
//    }

    for (t = 0; t < NUM_WRITE_THREADS; t++){ /* creating write threads */
        rc = pthread_create(&writeThreads[t], NULL, &writeFile, (void *)t);
        printf("Write Thread %d is created\n",t);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    //joining the threads
    for (t = 0; t < NUM_READ_THREADS; t++)
        pthread_join(readThreads[t], NULL);

    for (t = 0; t < NUM_UPPER_THREADS; t++)
        pthread_join(upperThreads[t], NULL);

//    for (t = 0; t < NUM_REPLACE_THREADS; t++)
//        pthread_join(replaceThreads[t], NULL);

    for (t = 0; t < NUM_WRITE_THREADS; t++)
        pthread_join(writeThreads[t], NULL);

    //destroying the mutex
    pthread_mutex_destroy(&lock);
    pthread_exit(NULL);
}
