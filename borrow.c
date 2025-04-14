#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#define TEMP_FILE "temp_user.txt"
int is_valid_bid(const char* bid);
int is_valid_date(const char* );
int updata_file(Lend_Return);

void run_borrow() {
    if (!is_logged_in) {
        printf("You must login first to borrow books.\n");
        return;
    }
    if (current_user.lendAvailable <= 0) {
        printf("You cannot borrow more books.\n");
        return;
    }
    run_search();
    char bid_input[MAX_BID];
    Lend_Return lend;
    while (1) {
        printf("Enter BID of the book to borrow > ");
        fgets(bid_input, sizeof(bid_input), stdin);
        trim(bid_input);

        // BID 유효성 검사
        if (!is_valid_bid(bid_input)) {
            continue;
        }
        // 검색된 책 중 BID 일치 여부 확인
        FILE* file = fopen(BOOK_FILE, "r");
        if (!file) {
            printf("Cannot open file.\n");
            return;
        }
        char status_char;
        int found = 0;
        Book book;
        printf("\n[Search Result]\n");

        while (fscanf_s(file, "%49[^,],%29[^,],%104[^,],%c\n",
            book.title, (unsigned)(sizeof(book.title)),
            book.author, (unsigned)(sizeof(book.author)),
            book.bid, (unsigned)(sizeof(book.bid)),
            &status_char, 1) == 4) {
            if (strcmp(book.bid, bid_input)) {
                found++;
                strcpy(lend.bookBid, book.bid);
                if (status_char == 'Y') {
                    printf("Error: 이미 대출된 책\n");
                    found++;
                }
                break;
            }
        }
        fclose(file);
        if (found == 0) {
            printf("Error: 해당하는 책 없음");
            continue;
        }
        else if (found == 2) {
            continue;
        }
        break;
    }

    char loan_date[MAX_DATE];
    while (1)
    {
        printf("Enter loan date > ");
        fgets(loan_date, sizeof(loan_date), stdin);
        trim(loan_date);
        if (!is_valid_date(loan_date)){
            continue;
        }
        strcpy(lend.borrowDate, loan_date);
        break;
    }

    printf("Do you really want to borrow this book? (Enter to confirm / No to cancel) > ");
    char confirm[5];
    fgets(confirm, sizeof(confirm), stdin);
    trim(confirm);

    if (strcasecmp(confirm, "No") == 0) {
        printf("Borrowing cancelled.\n");
        return;
    }
    if (!updata_file(lend)){
        return;
    }
}

int updata_file(Lend_Return lend) {
    //파일 수정
    FILE* file_user = fopen("USER_FILE", "a");
    FILE* file_temp = fopen("TEMP_FILE", "w");
    if (!file_user || !file_temp) {
        printf("Cannot open file.\n");
        return 0;
    }
    char name[100], studentId[20], password[20], bids[102];
    int available;
    char temp[102];
    while (fscanf(file_user, "%99[^,],%19[^,],%19[^,],%101[^,],%d\n",
        name, studentId, password, bids, &available) == 5) {
        if (strcmp(studentId, current_user.studentId) == 0) {
            // 이 사용자가 대상이면 수정된 정보로 기록
            strcat(bids, ";");
            strcat(bids,lend.bookBid);
            fprintf(file_temp, "%s,%s,%s,%s,%d\n", name, studentId, password, bids, available - 1);
        }
        else {
            // 그 외 사용자는 그대로 복사
            fprintf(file_temp, "%s,%s,%s,%s,%d\n", name, studentId, password, bids, available);
        }
    }
    fclose(file_user);
    fclose(file_temp);
    remove(USER_FILE);
    rename(TEMP_FILE, USER_FILE);
    FILE* file_books = fopen("BOOK_FILE", "r");
    FILE* file_temp = fopen("TEMP_FILE", "w");
    if (!file_books || !file_temp) {
        printf("Cannot open file.\n");
        return 0;
    }
    char title[50], author[30], bid[105];
    char isAvailable;
    while (fscanf(file_books, "%49[^,],%[^,],%29[^,],%104[^,],%c\n",
        title, author, bid, &isAvailable) == 4) {
        if (strcmp(bid, lend.bookBid) == 0) {
            // 이 사용자가 대상이면 수정된 정보로 기록
            fprintf(file_temp, "%s,%s,%s,%c\n", title, author, bid, "N");
        }
        else {
            // 그 외 사용자는 그대로 복사
            fprintf(file_temp, "%s,%s,%s,%s,%c\n", title, author, bid, isAvailable);
        }
    }
    fclose(file_books);
    fclose(file_temp);
    remove(BOOK_FILE);
    rename(TEMP_FILE, BOOK_FILE);
    FILE* file_lend = fopen("LEND_RETURN_FILE", "");
    if (!file_user) {
        printf("Cannot open file.\n");
        return 0;
    }
    fprintf(file_lend, "%s,%s,%s,%s,%c\n", current_user.studentId, lend.bookBid, lend.borrowDate,"0","N");
    fclose(file_lend);

    return 1;
}

int is_valid_bid(const char* bid) {
    int len = strlen(bid);
    if (strlen(bid) == 0 || strspn(bid, " \t") == strlen(bid)) {
        printf("Error: BID cannot be empty or whitespace.\n");
        return 0;
    }
    for (int i = 0; i < len; ++i) {
        char c = bid[i];

        // 허용 문자 검사
        if (isalnum(c) || c == '-' || c == '.' || c == ':') {
            continue;
        }

        // 공백 문자 중 스페이스가 아닌 것 검사
        if (isspace(c) && c != ' ') {
            printf(".!! Error: BID cannot contain tab/newline characters.\n");
            return 0;
        }

        // 허용되지 않은 특수문자
        if (!isalnum(c) && c != '-' && c != '.' && c != ':' && c != ' ') {
            printf(".!! Error: BID contains invalid character: '%c'\n", c);
            return 0;
        }
    }
    return 1; // 모든 조건 통과
}

int is_valid_date(const char* input) {
    char date[9] = "";  // yyyyMMdd
    int di = 0;

    // 구분자 제거하면서 숫자만 추출
    for (int i = 0; input[i] != '\0' && di < 8; i++) {
        if (isdigit(input[i])) {
            date[di++] = input[i];
        }
        else if (input[i] != '-' && input[i] != '/') {
            printf(".!! Error: Date contains invalid characters.\n");
            return 0;
        }
    }
    if (di != 8) {
        printf(".!! Error: Date must be exactly 8 digits (YYYYMMDD).\n");
        return 0;
    }

    // 연도, 월, 일 추출
    int year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
        (date[2] - '0') * 10 + (date[3] - '0');
    int month = (date[4] - '0') * 10 + (date[5] - '0');
    int day = (date[6] - '0') * 10 + (date[7] - '0');

    // 유효한 날짜 범위인지 확인
    if (month < 1 || month > 12) {
        printf(".!! Error: Invalid month in date.\n");
        return 0;
    }

    int max_days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        max_days[1] = 29;  // 윤년
    }

    if (day < 1 || day > max_days[month - 1]) {
        printf(".!! Error: Invalid day in date.\n");
        return 0;
    }
    return 1; // 모두 통과
}