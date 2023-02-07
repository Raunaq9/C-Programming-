// Wordle-Helper

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <csse2310a1.h>

#define USAGE_ERR 1
#define PATTERN_ERR 2
#define FILE_ERR 3
#define NO_MATCH 4
#define DEFAULT_LEN 5
#define LINE_LEN 50

struct Arguments {
    char* argType;
    int argLen;
    char* argWith;
    char* argWithout;
    char* argPattern;
    char* filePath;
};

// Variable for storing command line args.
struct Arguments arguments;

// Error Handling

// Handles command line usage error 
void usage_err() {
    fprintf(stderr, "Usage: wordle-helper [-alpha|-best] [-len len]"
            " [-with letters] [-without letters] [pattern]\n");
    exit(USAGE_ERR);
}

// Handles pattern error 
void pattern_err(int patternLen) {
    fprintf(stderr, "wordle-helper: pattern must be of length %d and"
            " only contain underscores and/or letters\n", patternLen);
    exit(PATTERN_ERR);
}

// Handles invalid file 
void invalid_file(char* fileName) {
    fprintf(stderr, "wordle-helper: dictionary"
            " file \"%s\" cannot be opened\n", fileName);
    exit(FILE_ERR);
}

// Checks if the length of the word is valid 
bool is_word_length_valid(char* arg) {
    if (strlen(arg) > 1 || isalpha(arg[0]) != 0) {
        return false;
    }
    int wordLen = atoi(arg);
    if ((wordLen < 4) || (wordLen > 9)) {
        return false;
    }
    return true;
}

// Checks if the string is valid by checking if its an alphabet
bool is_string_valid(char* str) {
    for (int i = 0; i < strlen(str); i++) {
        if (isalpha(str[i]) == 0) {
            return false;
        }
    }
    return true;
}

// Checks if the pattern length is equal 
// to the length of the word from dictionary
bool is_pattern_length_valid(char* pattern) {
    if ((strlen(pattern) == arguments.argLen)) {
        return true;
    } else {
        return false;
    }
}

// Checks if the pattern is valid 
// Checks if the pattern length is valid first 
bool is_pattern_valid(char* pattern) {
    if (is_pattern_length_valid(pattern)) {
        for (int i = 0; i < strlen(pattern); i++) {
            if (isalpha(pattern[i]) == 0 && pattern[i] != '_') {
                return false;
            }
        }
        return true;
    }
    return false;
}

// Checks if the file can be opened and is readable
bool is_file_openable(char* fileName) {
    FILE* wordleFile = fopen(fileName, "r");
    if (wordleFile == 0 || wordleFile == NULL) {
        return false;
    }
    fclose(wordleFile);
    return true;
}

// Compares if two words have the same length
bool compare_word_len(int wordLen, char* fileWord) {
    int fileWordLen = strlen(fileWord);
    if (fileWordLen == wordLen) {
        return true;
    }
    return false;
}

// Converts a word to uppercase
char* word_to_upper(const char* word) {
    char* upper = (char *)malloc(strlen(word) * sizeof(char));
    strcpy(upper, word);
    int i = 0;
    while (word[i]) { 
        upper[i] = toupper(word[i]);
        i++; 
    } 
    return upper;
}

// Checks if the pattern is valid 
bool check_pattern(char* pattern, char* word) {
    if (pattern == NULL) {
        return true;
    }
    int patternLen = strlen(pattern);
    for (int i = 0; i < patternLen; i++) {
        if (!((pattern[i] == '_') || (toupper(pattern[i]) 
                == toupper(word[i])))) {
            return false;
        }
    }
    return true;
}

// Checks if the '-with' argument are present in the word
bool check_with(char* parsedWord, char* argWith) {
    char* withLetter = word_to_upper(argWith);
    char* word = (char *)malloc(strlen(parsedWord) * sizeof(char));
    strcpy(word, parsedWord);

    for (int i = 0; i < strlen(withLetter); i++) {
        char* temp = strchr(word, withLetter[i]);
        if (temp == NULL) {
            return false;
        } else {
            word[temp - word] = '.';
        }
    }
    return true;
}

// Checks if the '-without' argument are not present in the word
bool check_without(char* parsedWord, char* argWithout) {
    char* withoutLetter = word_to_upper(argWithout);
    char* word = (char *)malloc(strlen(parsedWord) * sizeof(char));
    strcpy(word, parsedWord);
    for (int i = 0; i < strlen(withoutLetter); i++) {
        char* temp = strchr(word, withoutLetter[i]);
        if (temp != NULL) {
            return false;
        }
    }
    return true;
}

// Checks if the specified argument type is present
void check_arg_type(char* argType) {
    if (argType != NULL) {
        usage_err();
    }
}

// Checks if the argument type pattern is present
void check_arg_pattern(char* argPattern, int argLen) {
    if (argPattern != NULL) {
        if (is_pattern_valid(argPattern) == false) {
            pattern_err(argLen);
        }
    } 
}

// Processes the command line arguments
void process_arguments(int argc, char** argv) {
    arguments.argLen = DEFAULT_LEN;
    for (int i = 1; i < argc; i++) {
        char* curr = argv[i];
        if (curr[0] == '-') {
            if (strcmp(curr, "-alpha") == 0 || strcmp(curr, "-best") == 0) {
                check_arg_type(arguments.argType);
                arguments.argType = argv[i];
                continue;
            } 
            if ((strcmp(curr, "-len") == 0) && ((i + 1) < argc)) {
                if (is_word_length_valid(argv[i + 1]) == false) {
                    usage_err();
                }
                int wordLen = atoi(argv[i + 1]);
                arguments.argLen = wordLen;
                i++;
                continue;
            }
            if ((strcmp(curr, "-with") == 0) && ((i + 1) < argc)) {
                check_arg_type(arguments.argWith);
                if (is_string_valid(argv[i + 1]) == false) {
                    usage_err();
                }
                arguments.argWith = argv[i + 1];
                i++;
                continue;
            }
            if ((strcmp(curr, "-without") == 0) && ((i + 1) < argc)) {
                check_arg_type(arguments.argWithout);
                if (is_string_valid(argv[i + 1]) == false) {
                    usage_err();
                }
                arguments.argWithout = argv[i + 1];
                i++;
                continue;
            } else {
                usage_err();
            }
        }
        if (arguments.argPattern == NULL) {
            arguments.argPattern = argv[i];  
        } else {
            usage_err();
        }
    }
    check_arg_pattern(arguments.argPattern, arguments.argLen);
}

// Sets files path by either using the default path or 
// using the set environment
void set_file_path() {
    if (getenv("WORDLE_DICTIONARY") == NULL) {
        arguments.filePath = "/usr/share/dict/words";
    } else {
        arguments.filePath = getenv("WORDLE_DICTIONARY");
    }
    if (is_file_openable(arguments.filePath) == false) {
        invalid_file(arguments.filePath);
    }
}

// Returns all the words in an array in upper case.
char** process_file(char* fileName, int* fileLen) {
    // Opening file to read
    FILE* file = fopen(fileName, "r");
    int i = 0;
    int c = 100;
    int arrLen = 100;
    char** arr = (char **)malloc(arrLen * sizeof(char *));

    char line[LINE_LEN];

    // getting each character of file
    while (fgets(line, LINE_LEN, file)) {
        if (i == arrLen) {
            arrLen += c;
            char** temp = realloc(arr, arrLen * sizeof(char *));
            arr = temp;
        }

        line[strlen(line) - 1] = '\0';
        char* string = (char *)malloc(LINE_LEN * sizeof(char));
        strcpy(string, line);
        arr[i] = string;
        i++;
    }
    *fileLen = i;
    return arr;
}

// Process wordleData without sorting type
char** wordle_helper(char** wordleData, int arrLen, int* len) {
    int outputArrLen = 50;
    int incrementer = 50;
    char** outputArr = (char **)malloc(outputArrLen * sizeof(char *));
    ;
    int counter = 0;

    for (int i = 0; i < arrLen; i++) {
        bool equal = (compare_word_len(arguments.argLen, wordleData[i]) 
            && check_pattern(arguments.argPattern, wordleData[i]) 
            && is_string_valid(wordleData[i]));
        if (equal == true) {
            if (counter == outputArrLen) {
                outputArrLen += incrementer;
                char** newlines = realloc(outputArr, outputArrLen *
                        sizeof(char *));
                outputArr = newlines;
            }
            outputArr[counter] = word_to_upper(wordleData[i]);
            counter++;
        }
    }
    *len = counter;
    return outputArr;
}

// Function for constant of qsort sorting for alpha
int comparator(const void* word1, const void* word2) {
    const char* temp1 = *(const char* const *)word1;
    const char* temp2 = *(const char* const *)word2;
    char* upperWord1 = word_to_upper(temp1);
    char* upperWord2 = word_to_upper(temp2);
    int upperCmp = strcmp(upperWord1, upperWord2);
    if (upperCmp != 0) {
        return upperCmp;
    }
    return strcmp(temp1, temp2);
}

// Function for alpha sorting
void sort_by_alpha(char** wordsArr, int len) {
    qsort(wordsArr, len, sizeof(char *), comparator);
}

// Function for constant of qsort sorting for best
int best_const(const void* word1, const void* word2) {
    const char* temp1 = *(const char* const *)word1;
    const char* temp2 = *(const char* const *)word2;
    char* upperWord1 = word_to_upper(temp1);
    char* upperWord2 = word_to_upper(temp2);
    int guessCmp = guess_compare(upperWord1, upperWord2);
    if (guessCmp == 0) {
        return comparator(temp1, temp2);
    } else if (guessCmp > 0) {
        return 1;
    }
    return -1;
}

// Function for sorting if the best argument is present
void best_sort(char** input, int len) {
    qsort(input, len, sizeof(char *), best_const);
}

// Sorts the wordle array according to the '-with' arguments
void with_sort(char** wordsArr, int arrLen) {
    for (int i = 0; i < arrLen; i++) {
        if (check_with(wordsArr[i],arguments.argWith) == false) {
            wordsArr[i] = NULL;
        }
    }
}

// Sorts the wordle array according to the '-without' arguments
void without_sort(char** wordsArr, int arrLen) {
    for (int i = 0; i < arrLen; i++) {
        if (check_without(wordsArr[i],arguments.argWithout) == false) {
            wordsArr[i] = NULL;
        }
    }
}

// Sorts the wordle array according to the 
// -with and '-without' arguments if both are present
void with_without_sort(char** wordsArr, int arrLen) {
    for (int i = 0; i < arrLen; i++) {
        if (check_with(wordsArr[i],arguments.argWith) == false) {
            wordsArr[i] = NULL;
        } else if (check_with(wordsArr[i],arguments.argWith) == true) {
            if (check_without(wordsArr[i],arguments.argWithout) == false) {
                wordsArr[i] = NULL;
            }
        }
    }
}

// Removes duplicates from an array
void remove_duplicate(char** wordsArr, int arrLen) {
    for (int i = 1; i < arrLen; i++) {
        if (strcmp(wordsArr[i], wordsArr[i - 1]) == 0) {
            wordsArr[i] = NULL;
        }
    }
}

// Prints the words array
void print_words(char** words, int wordLen, int* counterLen) {
    int counter = 0;
    for (int i = 0; i < wordLen; i++) {
        if (words[i] != NULL) {
            printf("%s\n", words[i]);
            counter++;
        }   
    }
    *counterLen = counter;
}

// Start of `wordle-helper` program
int main(int argc, char** argv) {
    if (argc < 1 || argc > 9) {
        usage_err();
    }

    // Processes command line arguments
    process_arguments(argc, argv);

    // Checking whether file can be opened
    set_file_path();
    
    // Reading file
    int fileLen = 0;
    char** rawData = process_file(arguments.filePath, &fileLen);

    int outputArrLen = 0;
    char** outputArr = wordle_helper(rawData, fileLen, &outputArrLen);
    int printCounter = 0;
    if (arguments.argType != NULL) {
        if (strcmp(arguments.argType, "-alpha") == 0) {
            sort_by_alpha(outputArr, outputArrLen);
            remove_duplicate(outputArr, outputArrLen);
        } else if (strcmp(arguments.argType, "-best") == 0) {
            best_sort(outputArr, outputArrLen);
            remove_duplicate(outputArr, outputArrLen);
        }
    }
    if (arguments.argWith != NULL && arguments.argWithout == NULL) {
        with_sort(outputArr, outputArrLen);
    } else if (arguments.argWithout != NULL && arguments.argWith == NULL) {
        without_sort(outputArr, outputArrLen);
    } else if (arguments.argWithout != NULL && arguments.argWith != NULL) {
        with_without_sort(outputArr, outputArrLen);
    }
    
    print_words(outputArr, outputArrLen, &printCounter);
    if (printCounter == 0) {
        return NO_MATCH;
    }
    return 0;
}

