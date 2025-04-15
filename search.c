#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "common.h"
#define MAX_KEYWORDS 10

int run_search(int mode) {
    char input[100];
    char* keywords[MAX_KEYWORDS];
    int keyword_count = 0;

    printf("Enter search keyword > ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';
    trim(input);

    char* context = NULL;
    char* token = strtok_s(input, " \t", &context);
    while (token && keyword_count < MAX_KEYWORDS) {
        keywords[keyword_count++] = token;
        token = strtok_s(NULL, " \t", &context);
    }

    linked_list* book_list = read_book_data();
    node* current = book_list->head;
    int found = 0;

    printf("\n[Search Result]\n");

    while (current) {
        Book* book = (Book*)current->data;

        int match_all = 1;
        for (int i = 0; i < keyword_count; ++i) {
            if (!(strstr(book->title, keywords[i]) ||
                strstr(book->author, keywords[i]) ||
                strcmp(book->bid, keywords[i]) == 0)) {
                match_all = 0;
                break;
            }
        }

        if (match_all) {
            if (mode == 1) {
                printf("> Title: %s\n  Author: %s\n  BID: %s\n\n", book->title, book->author, book->bid);
                found = 1;
            }
            else if (mode == 0) {
                printf("> Title: %s\n  Author: %s\n  BID: %s\n  Availibility: %c\n\n", book->title, book->author, book->bid, book->isAvailable);
                found = 1;
            }
        }

        current = current->next;
    }

    if (!found) {
        printf(".!! No search results.\n\n");
        if (mode == 1) {
            run_search(1);
        }
        else if (mode == 0) {
            return 0;
        }
    }

    return 1;
}