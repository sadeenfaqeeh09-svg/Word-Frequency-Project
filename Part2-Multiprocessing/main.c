#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#define MaxWords 18111000 // MAX NUM OF WORDS IN THE FILE
#define MaxUniqueWords 300000   //MAX NUM OF UNIQUE WORDS
#define SharedMemoSize 1024 * 1024 * 50 //SHARED MEMORY SIZE (50 MB)

typedef struct {
    char word[50]; //WORD STRING=> MAX 50 CHAR
    int count; //FREQUENCY COUNT OF THE WORD
} WordFreq;
typedef struct { //TO HOLD THE DATA FOR EACH CHILD PROCESS
    char** words;//ARRAY OF WORDS
    int start;  //STARTING INDEX FOR THE CHILD PROC
    int end; // ENDING INDEX FOR THE CHILD PROC
} ProcessData;

WordFreq* sharedWords;  //SHARED MEMORY FOR WORD FREQUENCY TABLE
int* sharedUniqueCount; //SHARED MEMORY FOR UNIQUE WORD COUNT
sem_t* semaphore; // SEMAPHORE TO PREVENT RACE CONDITIONS

void addWordToSharedMemo(const char* word);
void countWords(ProcessData data);
void merge(WordFreq* arr, int left, int mid, int right);
void mergeSort(WordFreq* arr, int left, int right);

int main() {
    struct timeval start, end;
    FILE* file = fopen("cleanedUpFile.txt", "r");
    if (!file) {
        perror("CAN'T OPEN THE FILE :( ");
        return 1;
    }
//READ ALL WORDS INTO MEMORY DYNAM.
    char** words = (char*)malloc(MaxWords * sizeof(char));//ALLOCATE MEMO. FOR WORDS ARRAY
    int wordCount = 0; //COUNTER FOR THE TOTAL NUM OF WORDS
    char word[50]; // TO HOLD INDIVIDUAL WORDS
    while (fscanf(file, "%49s", word) != EOF) {
        words[wordCount] = strdup(word);//STORE EACH WORD IN THE ARRAY
        wordCount++;
    }
    fclose(file);
    gettimeofday(&start, NULL);
    int sharedMemoUnique = shmget(IPC_PRIVATE, SharedMemoSize, IPC_CREAT | 0666);//CREATE SHARED MEMORY FOR WORDS
    if (sharedMemoUnique < 0) {
        perror("ERROR HANDLING FOR SHARED MEMORY ALLOCATION :( ");
        return 1;
    }
    sharedWords = (WordFreq*)shmat(sharedMemoUnique, NULL, 0);  // ATTACH THE SHARED MEMORY FOR WORDS
    if (sharedWords == (void*)-1) {                                  // ERROR HANDLING FOR MEMORY ATTACHMENT
        perror("Shared memory attachment failed");
        return 1;
    }
    int uniqueCount_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666); //SHARED MEMORY FOR THE UNIQUE WORD COUNT
    if (uniqueCount_id < 0) {
        perror("ERROR HANDLING FOR UNIQUE COUNT MEMORY ALLOCATION:( ");
        return 1;
    }
    sharedUniqueCount = (int*)shmat(uniqueCount_id, NULL, 0);//ATTACH THE SHARED MEMORY FOR UNIQUE COUNT
    if (sharedUniqueCount == (void*)-1) {
        perror("ERROR HANDLING FOR MEMORY ATTACHMENT");
        return 1;
    }
    *sharedUniqueCount = 0;  //INIT. THE UNIQUE WORD COUNT TO ZEROOO
    sem_t semaphore_instance; //CREATE A SEMAPHORE INSTANCE
    semaphore = &semaphore_instance;
    if (sem_init(semaphore, 1, 1) != 0) {
        perror("ERROR IN INITIALIZE :( ");
        return 1;
    }
    int numProcesses = 8; //NUM OF CHILD WE NEED TO DO 2 4 6 8
    int chunkSize = wordCount / numProcesses;//DIVIDE THE WORD INTO CHUNKS FOR EACH PROCESS
    pid_t pids[numProcesses]; //TO STORE PROCESS IDS
    printf("Number of child processes = %d\n", numProcesses);
//NOW FORK CHILD PROCESSES AND ASSIGN EACH A CHUNK OF WORDS TO PROCESS
    for (int i = 0; i < numProcesses; i++) {
        int startIdx = i * chunkSize;
        int endIdx = (i == numProcesses - 1) ? wordCount : (i + 1) * chunkSize; //ENDING INDEX
        if ((pids[i] = fork()) == 0) {//START CHILD P.
            ProcessData data;
            data.words = words;
            data.start = startIdx;// NOW SET THE START INDEX FOR THIS PROCESS
            data.end = endIdx;//THEN SET THE END INDEX FOR THIS PROCESS
            countWords(data);//COUNT WORD FREQUENCIES FOR THE ASSIGNED CHUNK
        }
    }
    for (int i = 0; i < numProcesses; i++) {
        wait(NULL);  // WAIT FOR EACH CHILD PROCESS TO FINISH
    }
    mergeSort(sharedWords, 0, *sharedUniqueCount - 1);
    gettimeofday(&end, NULL);
    printf("Top 10 most frequent words:\n");
    for (int i = 0; i < 10 && i < *sharedUniqueCount; i++) {
        printf("%s: %d\n", sharedWords[i].word, sharedWords[i].count);
    }
    // CALCULATE AND PRINT THE ELAPSED TIME
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0; //CONVERT SECONDS TO MILLISECONDS
    elapsed += (end.tv_usec - start.tv_usec) / 1000.0;           //CONVERT MICROSECONDS TO MILLISECONDS
    printf("Execution time: %.2f ms\n", elapsed);
//CLEAN UP SHARED MEMO. AND SEMAPHORE
    shmdt(sharedWords);//DETACH SHARED MEMORY FOR WORDS
    shmdt(sharedUniqueCount); //DETACH SHARED MEMORY FOR UNIQUE COUNT
    shmctl(sharedMemoUnique, IPC_RMID, NULL);//REMOVE THE SHARED MEMORY FOR WORDS
    shmctl(uniqueCount_id, IPC_RMID, NULL);//REMOVE THE SHARED MEMORY FOR UNIQUE COUNT
    sem_destroy(semaphore);//END IT
    for (int i = 0; i < wordCount; i++) free(words[i]);     // FREE DYNAMICALLY ALLOCATED MEMORY FOR WORDS
    free(words);
    return 0;
}

// FUNCTION TO ADD A WORD TO SHARED MEMORY, SYNCHRONIZING ACCESS USING SEMAPHORE
void addWordToSharedMemo(const char* word) {
    sem_wait(semaphore); // LOCK THE SEMAPHORE TO PREVENT RACE CONDITIONS
    for (int i = 0; i < *sharedUniqueCount; i++) { // IF ALREADY EXISTS
        if (strcmp(sharedWords[i].word, word) == 0) {
            sharedWords[i].count++;
            sem_post(semaphore); //UNLOCK THE SEMAPHORE
            return;
        }
    }
    strcpy(sharedWords[*sharedUniqueCount].word, word);    // IF THE WORD IS NEW, ADD IT TO THE SHARED MEMORY
    sharedWords[*sharedUniqueCount].count = 1;
    (*sharedUniqueCount)++; //INCR. THE UNIQUE WORD COUNT
    sem_post(semaphore); //UNLOCK THE SEMAPHORE
}

// FUNCTION EXECUTED BY EACH CHILD PROCESS TO COUNT WORD FREQUENCIES IN ITS CHUNK
void countWords(ProcessData data) {
    for (int i = data.start; i < data.end; i++) {
        addWordToSharedMemo(data.words[i]);//ADD THE WORD TO SHARED MEMORY
    }
    exit(0);
}

// MERGE FUNCTION TO MERGE TWO SORTED SUBARRAYS
void merge(WordFreq* arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    WordFreq* L = (WordFreq*)malloc(n1 * sizeof(WordFreq));
    WordFreq* R = (WordFreq*)malloc(n2 * sizeof(WordFreq));
    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int i = 0; i < n2; i++) R[i] = arr[mid + 1 + i];
    int i = 0, j = 0;
    int k = left;
    while (i < n1 && j < n2) {
        if (L[i].count >= R[j].count)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L);
    free(R);
}

// MERGE SORT
void mergeSort(WordFreq* arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}
