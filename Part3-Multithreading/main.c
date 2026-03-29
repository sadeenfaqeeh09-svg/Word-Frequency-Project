#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#define MaxWords 18111000       // MAX NUMBER OF WORDS IN THE FILE
#define MaxUniqueWords 300000   // MAX NUMBER OF UNIQUE WORDS
#define SharedMemoSize 1024 * 1024 * 50 // SHARED MEMORY SIZE (50 MB)
#define MaxThreads 8 //MAX NUM OF THREADS
typedef struct {
    char word[50];
    int count;  // COUNT OF THE WORD
} WordFreq;
typedef struct {
    char** words;  // ARRAY OF WORDS
    int start;
    int end;
    WordFreq* localWords; /
    int* localCount; // NUMBER OF UNIQUE WORDS PROCESSED BY THE THREAD
} ThreadData;

WordFreq* sharedWords;   // SHARED MEMORY FOR WORD FREQUENCY TABLE
int* sharedUniqueCount;   // SHARED MEMORY FOR UNIQUE WORD COUNT
pthread_mutex_t mutex;     // MUTEX TO PREVENT RACE CONDITIONS ON SHARED DATA

// FUNCTION TO ADD A WORD TO THE LOCAL FREQUENCY TABLE
void addWordLocal(WordFreq* localWords, int* localCount, const char* word) {
    for (int i = 0; i < *localCount; i++) { // ITERATE OVER LOCAL FREQUENCY TABLE
        if (strcmp(localWords[i].word, word) == 0) { // IF WORD EXISTS
            localWords[i].count++; // INCREMENT COUNT
            return;
        }
    }
    strcpy(localWords[*localCount].word, word);  // IF WORD DOES NOT EXIST, ADD IT
    localWords[*localCount].count = 1;
    (*localCount)++;//INCREMENT LOCAL COUNT
}

// FUNCTION TO MERGE LOCAL WORD COUNTS INTO SHARED MEMORY
void mergeLocalToShared(WordFreq* localWords, int localCount) {
    pthread_mutex_lock(&mutex); // LOCK SHARED MEMORY TO PREVENT RACE CONDITIONS
    for (int i = 0; i < localCount; i++) {
        int found = 0;
        for (int j = 0; j < *sharedUniqueCount; j++) {
            if (strcmp(sharedWords[j].word, localWords[i].word) == 0) { // IF WORD EXISTS
                sharedWords[j].count += localWords[i].count; // INCREMENT SHARED COUNT
                found = 1;
                break;
            }
        }
        if (!found && *sharedUniqueCount < MaxUniqueWords) { // IF WORD DOES NOT EXIST, ADD IT TO THE SHARED MEMORY
            strcpy(sharedWords[*sharedUniqueCount].word, localWords[i].word);
            sharedWords[*sharedUniqueCount].count = localWords[i].count;
            (*sharedUniqueCount)++; // INCREMENT UNIQUE WORD COUNT
        }
    }
    pthread_mutex_unlock(&mutex);//UNLOCK SHARED MEMORY
}

// FUNCTION EXECUTED BY EACH THREAD
void* countWords(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start; i < data->end; i++) { // PROCESS WORDS IN THE ASSIGNED RANGE
        addWordLocal(data->localWords, data->localCount, data->words[i]);
    }
    mergeLocalToShared(data->localWords, *data->localCount);
    free(data->localWords);
    free(data);  // FREE THREAD DATA
    return NULL;
}

// FUNCTION TO MERGE TWO SUBARRAYS FOR MERGE SORT
void merge(WordFreq* arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    WordFreq* L = (WordFreq*)malloc(n1 * sizeof(WordFreq));
    WordFreq* R = (WordFreq*)malloc(n2 * sizeof(WordFreq));
    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int i = 0; i < n2; i++) R[i] = arr[mid + 1 + i];
    int i = 0, j = 0, k = left;
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

// FUNCTION TO SORT WORD FREQUENCY ARRAY USING MERGE SORT
void mergeSort(WordFreq* arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

int main() {
    struct timeval start, end; // FOR TIMING EXECUTION
    FILE* file = fopen("cleanedUpFile.txt", "r");
    if (!file) {
        perror("FAILED TO OPEN FILE");
        return 1;
    }
    char** words = (char**)malloc(MaxWords * sizeof(char*));// READ ALL WORDS INTO MEMORY
    int wordCount = 0;
    char word[50];
    while (fscanf(file, "%49s", word) != EOF) {
        words[wordCount] = strdup(word); // DUPLICATE WORD INTO MEMORY
        wordCount++;
    }
    fclose(file);
    gettimeofday(&start, NULL);
    sharedWords = (WordFreq*)malloc(MaxUniqueWords * sizeof(WordFreq));// ALLOCATE SHARED MEMORY FOR WORD FREQUENCY DATA
    sharedUniqueCount = (int*)malloc(sizeof(int));
    *sharedUniqueCount = 0; //INIT. UNIQUE WORD COUNT
    pthread_mutex_init(&mutex, NULL);// INITIALIZE THE MUTEX
    int numThreads = 2; //FOR THREDS 2 4 6 8
    int chunkSize= wordCount/numThreads;
    pthread_t threads[numThreads];
    printf("Number of threads = %d\n", numThreads);
    for (int i = 0; i < numThreads; i++) {
        int startIdx = i * chunkSize;
        int endIdx = (i == numThreads - 1) ? wordCount : (i + 1) * chunkSize;
        ThreadData* data = (ThreadData*)malloc(sizeof(ThreadData));
        data->words = words;
        data->start = startIdx;
        data->end = endIdx;
        data->localWords = (WordFreq*)malloc(MaxUniqueWords * sizeof(WordFreq));
        data->localCount = (int*)malloc(sizeof(int));
        *data->localCount = 0;
        pthread_create(&threads[i], NULL, countWords, (void*)data);// CREATE THREAD
    }
    for (int i = 0; i < numThreads; i++) { // WAIT FOR ALL THREADS TO COMPLETE
        pthread_join(threads[i], NULL);
    }
    mergeSort(sharedWords, 0, *sharedUniqueCount - 1);
    gettimeofday(&end, NULL);
    printf("Top 10 most frequent words:\n");
    for (int i = 0; i < 10 && i < *sharedUniqueCount; i++) {
        printf("%s: %d\n", sharedWords[i].word, sharedWords[i].count);
    }
//TO CALC AND PRINT ELAPSED TIME
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0; // CONVERT SECONDS TO MILLISECONDS
    elapsed += (end.tv_usec - start.tv_usec) / 1000.0; // CONVERT MICROSECONDS TO MILLISECONDS
    printf("Execution time: %.2f MS\n", elapsed);
    free(sharedWords);// CLEAN UP
    free(sharedUniqueCount);
    pthread_mutex_destroy(&mutex); //END MUTEX
    for (int i = 0; i < wordCount; i++) free(words[i]);
    free(words);
    return 0;
}
