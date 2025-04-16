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

    // 키워드 분리
    char* context = NULL;
    char* token = strtok_s(input, " \t", &context);
    while (token && keyword_count < MAX_KEYWORDS) {
        keywords[keyword_count++] = token;
        token = strtok_s(NULL, " \t", &context);
    }

    FILE* file = fopen(BOOK_FILE, "r");
    if (!file) {
        printf("도서 정보 파일을 열 수 없습니다.\n");
        return 0;
    }

    //fileio.c 이용하기 전
    Book book;
    char status_char;
    int found = 0;

    printf("\n[Search Result]\n");

    while (fscanf_s(file, "%49[^,],%29[^,],%104[^,],%c\n",
        book.title, (unsigned)sizeof(book.title),
        book.author, (unsigned)sizeof(book.author),
        book.bid, (unsigned)sizeof(book.bid),
        &status_char, 1) == 4) {

        book.isAvailable = (status_char == 'Y') ? 'Y' : 'N';

        int match_all = 1;
        for (int i = 0; i < keyword_count; ++i) {
            if (!(strstr(book.title, keywords[i]) ||
                strstr(book.author, keywords[i]) ||
                strcmp(book.bid, keywords[i]) == 0)) {
                match_all = 0;
                break;
            }
        }

        if (match_all) {
            if (mode == 1) {
                printf("> Title: %s\n  Author: %s\n  BID: %s\n\n", book.title, book.author, book.bid);
            }
            else {
                printf("> Title: %s\n  Author: %s\n  BID: %s\n  Availibility: %s\n\n",
                    book.title, book.author, book.bid,
                    book.isAvailable == 'Y' ? "Y" : "N");
            }
            found = 1;
        }
    }

    fclose(file);

    if (!found) {
        printf(".!! No search results.\n\n");
        if (mode == 1) {
            return run_search(1); // 재귀 다시 입력
        }
        else {
            return 0;
        }
    }

    return 1;
}