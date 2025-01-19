
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


#define SPACE_CHAR ' '

//prototypes for functions to handle required functionality
// TODO: #1 What is the purpose of providing prototypes for
//          the functions in this code module
// ANSWER: 
        //Function prototypes are used to declare a function before it is defined, allowing the compiler to know the function's name, 
        //return type, and parameter types. This ensures that the compiler can check for correct usage of the function, preventing errors caused 
        //by incorrect assumptions about the function's signature. By providing explicit prototypes, you make it clear what the function expects 
        //and returns, even before the full function definition is encountered, which helps avoid mistakes in function calls, such as passing the 
        //wrong number or type of arguments. This improves code clarity and allows for better error checking.

void  usage(char *);
int   count_words(char *);
void  reverse_string(char *);
void  word_print(char *);


void usage(char *exename){
    printf("usage: %s [-h|c|r|w] \"string\" \n", exename);
    printf("\texample: %s -w \"hello class\" \n", exename);
}

//count_words algorithm
//  1.  create a boolean to indicate if you are at the start of a word
//      initialize to false
//  2.  Loop over the length of the string
//      2a.  Get the current character aka str[i]
//      2b.  Is word_start state false?
//           -  Is the current character a SPACE_CHAR?
//              * if YES, continue loop (a.k.a) goto top with "continue;"
//              * if NO, we are at the start of a new word
//                > increment wc
//                > set word_start to true
//      2c. Else, word_start is true
//          - Is the current character a SPACE_CHAR?
//            * if YES we just ended a word, set word_start to false
//            * if NO, its just a character in the current word so
//                     there is nothing more to do
//  3.  The current word count for the input string is in the wc variable
//      so just 'return wc;' 

int count_words(char *str){
    // Suggested local variables
    int len; // length of string
    int wc; //  word count
    bool word_start; // to count the start of the word

    // Please implement
    //initializing all three local variables
    word_start = false; //initializing to false
    len = strlen(str); // Using C based strlen, to get the length of string
    wc = 0;

    for (int i = 0; i < len; i++) {
        char ch = str[i];

        if (word_start == false) // if its not the start of word
        {
            if (ch != SPACE_CHAR) // if character is not a space
            {
                wc++;   //increment word count by 1
                word_start = true; //shows that we are at the start of the word
            }
            else
                continue; // if character is a space, we continue to the next block of code
        }
        else 
        {
           if (ch == SPACE_CHAR) // if it is the start of word
                word_start = false; 
        }
        
    }

    return wc;
}

//reverse_string() algorithm
//  1.  Initialize the start and end index variables
//      a.  end_idx is the length of str - 1.  We want to remove one
//          becuase at index str[len(str)] is the '\0' that we want
//          to preserve because we are using C strings.  That makes
//          the last real character in str as str[len(str)-1]
//      b.  start_idx is 0, thus str[0] is the first character in the
//          string.
//
//  2.  Loop while end_idx > start_idx
//      2a. swap the characters in str[start_idx] and str[end_idx]
//      2b. increment start_idx by 1
//      2c. decrement end_indx by 1
//
//  3. When the loop above terminates, the string should be reversed in place

void reverse_string(char *str) {
    // Suggested local variables
    int end_idx;        //should be length of string - 1
    int start_idx;      
    char tmp_char;

    // Please implement
    // Initializing local variables
    end_idx = strlen(str)-1;  //last character index
    start_idx = 0;   //starting index is set to 0

    while(end_idx > start_idx) {
        // swapping characters
        tmp_char = str[start_idx];
        str[start_idx] = str[end_idx];
        str[end_idx] = tmp_char;

        // increment front letters by 1 and decrement last letters by 1 until they meet in the middle
        start_idx++;
        end_idx--;

    }

    return;
}

//word_print() - algorithm
//
// Start by copying the code from count words.  Recall that that code counts
// individual words by incrementing wc when it encounters the first character 
// in a word.
// Now, at this point where we are incrementing wc we need to do a few more things
//      1. incrment wc, and set word_start to true like before
//      2. Now, set wlen to zero, as we will be counting characters in each word
//      3. Since we are starting a new word we can printf("%d. ", wc);
//
// If word_start is true, we are in an active word, so each time through the loop
// we would want to:
//      1. Check if the current character is not a SPACE_CHARACTER
//         a.  IF it is NOT A SPACE -> print the current character, increment wlen
//
//      2.  In the loop there are 2 conditions that indicate a current word is ending:
//          a. word_start is false and the current character is a SPACE_CHARACTER
//                  OR
//          b. the current loop index is the last character in the string (aka the
//             loop index is last_char_idx) 
//
//          IF either of these conditions are true:
//              * Print the word length for current word - printf(" (%d)\n", wlen);
//              * Set word_start to false
//              * Set wlen to 0 given we are starting a new word
//
// EXAMPLE OUTPUT
// ==============
// ./stringfun -w "C programming is fun"
// Word Print
// ----------
// 1. C (1)
// 2. programming (11)
// 3. is (2)
// 4. fun (3)

void  word_print(char *str){
    //suggested local variables
    int len;            //length of string - aka strlen(str);
    int last_char_idx;  //index of last char - strlen(str)-1;
    int wc = 0;         //counts words
    int wlen = 0;       //length of current word
    bool word_start = false;    //am I at the start of a new word

    // Please implement
    len = strlen(str);
    last_char_idx = len -1;

    for (int i = 0; i < len; i++) {
        char ch = str[i];

        // Check if we are at the beginning of a new word
        if (word_start == false) 
        {
            if (ch != SPACE_CHAR)   
            {
                wc++;               // Increment word count
                word_start = true;  // Set word_start
                wlen = 0;           // Reset the word length
                printf("%d. ", wc); // Print the word number
            }    
            else
                continue;  // if character is a space, we continue to the next block of code
        }

        // If we are inside a word
        if(word_start == true){
            if (ch != SPACE_CHAR)  //If there is no space character
            {
                printf("%c", ch);
                wlen++;
            } 
            else  //If space character is found 
            {
                printf(" (%d)\n", wlen); //Print word length
                word_start = false;  //Reset word_start
                wlen = 0;  //Reset word length
            } 
        }

        //Handling the last word if it's not followed by a space
        if (i == last_char_idx && word_start == true) {
            printf(" (%d)\n", wlen);
            //Added the last two to reset word_start and length but the code was working even without them
            word_start = false;
            wlen = 0;
        }
        
    }

    return;
}


int main(int argc, char *argv[]){
    char *input_string;     //holds the string provided by the user on cmd line
    char *opt_string;       //holds the option string in argv[1]
    char opt;               //used to capture user option from cmd line

    //THIS BLOCK OF CODE HANDLES PROCESSING COMMAND LINE ARGS
    if (argc < 2){
        usage(argv[0]);
        exit(1);
    }
    opt_string = argv[1];

    //note arv[2] should be -h -r -w or -c, thus the option is
    //the second character and a - is the first char
    if((opt_string[0] != '-') && (strlen(opt_string) != 2)){
        usage(argv[0]);
        exit(1);
    }

    opt = opt_string[1];   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //Finally the input string must be in argv[2]
    if (argc != 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2];
    //ALL ARGS PROCESSED - The string you are working with is
    //is the third arg or in arv[2]
    
    switch (opt){
        case 'c':
            int wc = 0;         //variable for the word count

            //TODO: #2. Call count_words, return of the result
            //          should go into the wc variable
            wc = count_words(input_string); //calling count_words 
            printf("Word Count: %d\n", wc);
            break;
        case 'r':
            //TODO: #3. Call reverse string using input_string
            //          input string should be reversed
            reverse_string(input_string);   //calling input_string
            printf("Reversed string: %s\n", input_string);

            //TODO:  #4.  The algorithm provided in the directions 
            //            state we simply return after swapping all 
            //            characters because the string is reversed 
            //            in place.  Briefly explain why the string 
            //            is reversed in place - place in a comment
            //ANSWER:
                    //The string is reversed in place because the reverse_string() function directly modifies the 
                    //original input_string array. It swaps characters within the existing memory allocated for the 
                    //string; it doesn't create a new string. Therefore, when the function returns, the original 
                    //input_string has been modified, and no further action is needed to "return" the reversed string.
            break;
        case 'w':
            printf("Word Print\n----------\n");

            //TODO: #5. Call word_print, output should be
            //          printed by that function
            word_print(input_string); //calling word_print
            break;

        //TODO: #6. In this code, the default case handles invalid command-line options passed to the program. If the user provides an 
                    //option other than 'c', 'r', or 'w', the default block executes, displaying a usage message and an error indicating 
                    //the invalid option. This ensures the program doesn't proceed with undefined behavior and provides helpful feedback 
                    //to the user before exiting with an error code.
        default:
            usage(argv[0]);
            printf("Invalid option %c provided, exiting!\n", opt);
            exit(1);
    }
    //TODO: #7. We placed break statements in each case prevent "fallthrough," where execution continues into the next case even if its 
                //condition isn't met. We don't need a break in default because it's the last case; there's nothing to fall through to. 
                //Forgetting a break would cause unintended execution of subsequent case blocks until a break is encountered or the switch
                //ends, leading to incorrect program behavior.
}
