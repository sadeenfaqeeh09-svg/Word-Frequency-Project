#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define MaxWordLength 100 //MAX LENGTH OF A WORD
#define TopTenFreqWords 10 //NUM OF TOP TEN FREQ. WORDS THAT SHOULD FIND AND PRINT

typedef struct { //TYPEDEF WordFrequency
    char word[MaxWordLength]; //ARRAY FOR WORD
    int count; //FOR OCCURRENCES
} WordFrequency;

char **ReadFileToArray(const char *filename, int *WordCount); //TO READ FILE INTO ARRAY OF WORDS
void FreeWordsToArray(char **words, int WordCount); //TO FREE ALLOCATED WORDS ARRAY
int FindTheIndexOfWordInFreqArray(WordFrequency *frequencies, int size, const char *word); //FIND INDEX FOR A WORD IN THE FREQ. ARRAY
void CountWordFreq(char **words, int WordCount, WordFrequency **frequencies, int *CountUnique); //TO COUNT WORD FREQ.
void MergeSortWordFreqMethod(WordFrequency *arr, int left, int right); //TO SORT THE ARRAY, USING WORD FREQ. MERGE SORTING
void MergeMethod(WordFrequency *arr, int left, int mid, int right);  //MERGE SORT
void PrintTopTenFreqWords(WordFrequency *frequencies, int CountUnique); //PRINT TOP TEN FREQ. WORD
double CalcTime(struct timeval StartP, struct timeval EndP); //CALC THE TIME

int main() {
    //TO START THE TIME
    struct timeval StartP, EndP;
    gettimeofday(&StartP, NULL);
    int WordCount = 0;//FOR NUM OF ALL WORDS
    char **words = ReadFileToArray("cleanedUpFile.txt", &WordCount);
    WordFrequency *frequencies = NULL; //POINTER TO AN ARRAY, TO STORE WORD FREQ.
    int CountUnique = 0; //FOR UNIQUE WORDS
    CountWordFreq(words, WordCount, &frequencies, &CountUnique);//COUNT THE FREQ. OF EACH WORD
    MergeSortWordFreqMethod(frequencies, 0, CountUnique - 1); //SORT DESCENDING
    PrintTopTenFreqWords(frequencies, CountUnique); //PRINT TOP 10
    FreeWordsToArray(words, WordCount);//TO FREE THE ALLOCATED ARRAY MEMO FOR WORDS ARRAY
    free(frequencies); //TO FREE THE ALLOCATED ARRAY MEMO FOR FREQ. ARRAY
    gettimeofday(&EndP, NULL); //END THE TIME
    printf("Total program execution time: %.6f seconds\n", CalcTime(StartP, EndP)); // PRINT THE TOTAL EXECUTION TIME
    return 0;
}

//TO READ FILE INTO ARRAY OF WORDS
char **ReadFileToArray(const char *filename, int *WordCount) {
    FILE *file = fopen(filename, "r");
    if (!file) { //CHECK
        perror("CAN'T OPEN THE FILE :( ");
        exit(EXIT_FAILURE);//EXIT THE PROG. WITH ERROR
    }
    int SizeOfWord = 100000; //INIT. SIZE OF WORD ARRAY
    char **words = malloc(SizeOfWord * sizeof(char *)); //ALLOCATED MEMO FOR WORD ARRAY
    if (!words) { // CHECK
        perror("SOMETHING WRONG IN ALLOCATED MEMORY:( ");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    char ARR[MaxWordLength]; // TEMP ARRAY
    while (fscanf(file, "%99s", ARR) != EOF) { //READ
        if (*WordCount >= SizeOfWord) { //IF NEEDED TO SIZEOF
            SizeOfWord *= 2;
            words = realloc(words, SizeOfWord * sizeof(char *)); //RESIZE THE ARRAY
            if (!words) { //CHECKING
                perror("ERROR IN REALLOCATING MEMO FOR WORDS ARR.");
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }
        words[*WordCount] = strdup(ARR); // DUPLICATE THE WORD & STORE IT IN ARRAY
        if (!words[*WordCount]) {
            perror("ERROR ALLOCATING MEMO FOR A WORD");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        (*WordCount)++;
    }
    fclose(file);
    return words;
}

//TO FREE ALLOCATED WORDS ARRAY
void FreeWordsToArray(char **words, int WordCount) {
    for (int i = 0; i < WordCount; i++) { // TO GO THROUGH ALL WORDS
        free(words[i]);
    }
    free(words);//FREE MEMO ALL0CATED FOR WORD ARR.
}

//FIND INDEX FOR A WORD IN THE FREQ.
int FindTheIndexOfWordInFreqArray(WordFrequency *frequencies, int size, const char *word) {
    for (int i = 0; i < size; i++) { // TO GO THROUGH ALL  FREQ. WORDS
        if (strcmp(frequencies[i].word, word) == 0) { //CHECK FOR MATCHING WORDS
            return i;
        }
    }
    return -1; //IF NOT FOUND
}

//TO COUNT WORD FREQ.
void CountWordFreq(char **words, int WordCount, WordFrequency **frequencies, int *CountUnique) {
    int SizeOfWord = 1000; //INIT. SIZO FOR FREQ. ARRAY
    *frequencies = malloc(SizeOfWord * sizeof(WordFrequency));//ALLOC. FOR FREQ. ARRAY
    if (!*frequencies) {
        perror("ERROR ALLOCATING MEMORY FOR FRE. ARRAY");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < WordCount; i++) { // TO GO THROUGH ALL WORDS
        int index = FindTheIndexOfWordInFreqArray(*frequencies, *CountUnique, words[i]); // Find the index of the word in the frequencies array
        if (index != -1) { //IF ALREADY IN FREQ ARRAY
            (*frequencies)[index].count++;
        } else { //IF NOT FOUND
            if (*CountUnique >= SizeOfWord) { //IF NEEDS TO BE SIZED
                SizeOfWord *= 2;
                *frequencies = realloc(*frequencies, SizeOfWord * sizeof(WordFrequency));
                if (!*frequencies) {
                    perror("ERROR IN REALLOCATING MEMO FOR FREQ. ARRAY");
                    exit(EXIT_FAILURE);
                }
            }
            strcpy((*frequencies)[*CountUnique].word, words[i]); //TO FREQ. ARRAY
            (*frequencies)[*CountUnique].count = 1;
            (*CountUnique)++;
        }
    }
}

//TO SORT THE ARRAY, USING WORD FREQ. MERGE SORTING
void MergeSortWordFreqMethod(WordFrequency *arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        MergeSortWordFreqMethod(arr, left, mid);
        MergeSortWordFreqMethod(arr, mid + 1, right);
        MergeMethod(arr, left, mid, right);
    }
}

// MERGE SORT
void MergeMethod(WordFrequency *arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    WordFrequency *L = malloc(n1 * sizeof(WordFrequency));
    WordFrequency *R = malloc(n2 * sizeof(WordFrequency));
    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].count >= R[j].count) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L);
    free(R);
}

//PRINT TOP TEN FREQ. WORD
void PrintTopTenFreqWords(WordFrequency *frequencies, int CountUnique) {
    printf("Top %d most frequent words:\n", TopTenFreqWords);
    for (int i = 0; i < TopTenFreqWords && i < CountUnique; i++) {
        printf("%s: %d\n", frequencies[i].word, frequencies[i].count);
    }
}

//CALC THE TIME
double CalcTime(struct timeval StartP, struct timeval EndP) {
    return (EndP.tv_sec - StartP.tv_sec) + (EndP.tv_usec - StartP.tv_usec) / 1e6; //IN SEC
}
