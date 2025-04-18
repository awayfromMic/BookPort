#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "verify.h"

#ifndef USER_FILE
#define USER_FILE "users_data.txt"
#endif

// External variables
extern int is_logged_in;
extern User current_user;

// External functions
extern void run_login();
extern void run_logout();
extern void run_verify();
extern linked_list* read_user_data(bool* file_integrity);
extern linked_list* read_book_data();
extern linked_list* read_borrow_data();
extern bool update_file(const char* file_name, linked_list* list);
extern void remove_node(linked_list* list, void* target, int type);
extern int is_valid_password(const char* pw);

void flush_stdin() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}


char* get_myinfo_canonical_command(char* input) {
	struct {
		const char* synonyms[20];
		const char* canonical;
	} myinfo_table[] = {
		{ { "withdraw", "withdra", "withdr", "withd", "with", "wit", "wi", "w"}, "withdraw"},
		{ { "change", "chang", "chan", "cha", "ch", "c"}, "change"},
		{ { "manage", "manag", "mana", "man", "ma", "m"}, "manage"}
	};

	for (int i = 0; i < sizeof(myinfo_table) / sizeof(myinfo_table[0]); i++) {
		for (int j = 0; myinfo_table[i].synonyms[j]; j++) {
			if (strcmp(input, myinfo_table[i].synonyms[j]) == 0)
				return (char*)myinfo_table[i].canonical;
		}
	}
	return NULL;
}

char* get_manage_canonical_command(char* input) {
	struct {
		const char* synonyms[20];
		const char* canonical;
	} manage_table[] = {
		{ { "list", "lis", "li", "l"}, "list"},
		{ { "record", "recor", "reco", "rec", "re", "r"}, "record"}
	};

	for (int i = 0; i < sizeof(manage_table) / sizeof(manage_table[0]); i++) {
		for (int j = 0; manage_table[i].synonyms[j]; j++) {
			if (strcmp(input, manage_table[i].synonyms[j]) == 0)
				return (char*)manage_table[i].canonical;
		}
	}
	return NULL;
}

typedef struct {
	char date[9];
	char type[8];
	char bid[MAX_BID];
} RecordEntry;

int compare_record_entry(const void* a, const void* b) {
	return strcmp(((RecordEntry*)a)->date, ((RecordEntry*)b)->date);
}

void run_record() {
	int user_record_count = 0;
	linked_list* lend_returndata;
	lend_returndata = read_borrow_data();
	int lend_return_count = 0;
	for (node* count = lend_returndata->head; count != NULL; count = count->next) lend_return_count++;
	RecordEntry* user_record = (RecordEntry*)malloc(2 * lend_return_count * sizeof(RecordEntry));


	for (node* j = lend_returndata->head; j != NULL; j = j->next) {
		Lend_Return* l = (Lend_Return*)j->data;
		if (strcmp(current_user.studentId, l->userid) == 0) {	
			strcpy(user_record[user_record_count].date, l->borrowDate);
			strcpy(user_record[user_record_count].type, "borrow");
			strcpy(user_record[user_record_count].bid, l->bookBid);
			user_record_count++;
		if (l->returnDate[0] != '\0') {
			strcpy(user_record[user_record_count].date, l->returnDate);
			strcpy(user_record[user_record_count].type, "return");
			strcpy(user_record[user_record_count].bid, l->bookBid);
			user_record_count++;
			}
		}
	}
	qsort(user_record, user_record_count, sizeof(RecordEntry), compare_record_entry);
	if(user_record_count>0) printf("=> ");
	for (int i = 0; i < user_record_count; i++) {
		char* d = user_record[i].date;
		printf("[%s] %s %c%c/%c%c/%c%c\n",
			user_record[i].type,
			user_record[i].bid,
			d[2], d[3], d[4], d[5], d[6], d[7]);
	}
	free(user_record);
}

void run_manage() {
	char input[100];
	linked_list* bookdata;
	bookdata = read_book_data();

	while (1) {
		printf("BookPort: My info - Enter command >");
		if (!fgets(input, sizeof(input), stdin)) break;
		trim(input);

		char* cmd = get_manage_canonical_command(input);
		if (!cmd) {
			printf(".!! Error: Wrong command entered\n");
			continue;
		}

		if (strcmp(cmd, "list") == 0) {
			for (int i = 0; i < 5; i++) {
				if (strlen(current_user.lentBids[i]) == 0) continue;

				for (node* j = bookdata->head; j != NULL; j = j->next) {
					Book* b = (Book*)j->data;
					if (strcmp(b->bid, current_user.lentBids[i]) == 0) {
						printf("Title: %s\n", b->title);
						printf("Author: %s\n", b->author);
						printf("BID: %s\n", b->bid);
					}
				}
			}
			break;
		}
		else if (strcmp(cmd, "record") == 0) {
			run_record();
			break;
		}
	}
	return;
}

void run_change() {
    char new_pw[100];

    while (1) {
        printf("BookPort: Enter your new password >");
        if (!fgets(new_pw, sizeof(new_pw), stdin)) {
            // fgets 실패 (파일 끝이나 에러)
            flush_stdin();
            printf(".!! Error: Failed to read input. Try again.\n");
            continue;
        }

        // 입력이 너무 길어 '\n'이 안 들어왔다면 flush 필요
        if (!strchr(new_pw, '\n')) {
            flush_stdin();
            printf(".!! Error: Password must be 5 to 20 characters long.\n");
            continue;
        }

        new_pw[strcspn(new_pw, "\n")] = '\0'; // 개행 문자 제거

        int result = is_valid_password(new_pw);
        if (result != 0) {
            // 케이스별 오류 메시지
            switch (result) {
            case 1: printf(".!! Error: Password cannot be an empty string.\n"); break;
            case 2: printf(".!! Error: Password must be 5 to 20 characters long.\n"); break;
            case 3: printf(".!! Error: Password cannot contain whitespace.\n"); break;
            case 4: printf(".!! Error: Repeated characters over limit.\n"); break;
            case 5: printf(".!! Error: Password must include both letters and digits.\n"); break;
            default: printf(".!! Error: Invalid password format.\n"); break;
            }
            continue;
        }

        // 이전과 같은 비밀번호인지 확인
        if (strcmp(new_pw, current_user.password) == 0) {
            printf(".!! Error: Password is the same as before.\n");
            continue;
        }

        break; // 모든 조건 통과
    }

    bool ok = true;
    linked_list* list = read_user_data(&ok);
    if (!ok) {
        printf(".!! Error: Failed to load user data.\n");
        return; // 유저데이터 읽어오는거 실패하면 오류메시지 띄워도 되나? 사용흐름도에 이런게 있었나?
    }

    for (node* cur = list->head; cur; cur = cur->next) {
        User* u = (User*)cur->data;
        if (strcmp(u->studentId, current_user.studentId) == 0) {
            strcpy(u->password, new_pw);
            strcpy(current_user.password, new_pw);
            break;
        }
    }

    if (!update_file(USER_FILE, list)) {
        printf(".!! Error: Failed to update user data.\n");
        return; // 이것도 오류메시지 띄워도 되나?
    }

    printf("··· Your password successfully changed\n");
}

void run_withdraw() {
    printf("BookPort: Do you want to withdraw?(.../No) >");
    char input[10];
    if (!fgets(input, sizeof(input), stdin)) {
        flush_stdin();
    }
    else {
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "No") == 0) return;
        if (!strchr(input, '\n')) flush_stdin();
    }

    bool ok = true;
    linked_list* list = read_user_data(&ok);
    for (node* cur = list->head; cur; cur = cur->next) {
        User* u = (User*)cur->data;
        if (strcmp(u->studentId, current_user.studentId) == 0) {
            remove_node(list, u, 1);
            break;
        }
    }

    update_file(USER_FILE, list);
    run_verify();
    run_logout();
}

void run_myinfo() {
	if (is_logged_in == 1) {
		printf("[My Information]\n");
		printf("Name: %s\n", current_user.name);
		printf("ID: %s\n", current_user.studentId);

		char input[100];

		while (1) {
			printf("BookPort: My info - Enter command>");
			if(!fgets(input, sizeof(input), stdin)) break;
			trim(input);

			char* cmd = get_myinfo_canonical_command(input);
			if (!cmd) {
				printf(".!! Error: Wrong command entered\n");
				continue;
			}

			if (strcmp(cmd, "withdraw") == 0) {
				run_withdraw();
				break;
			}
			else if (strcmp(cmd, "change") == 0) {
				run_change();
				break;
			}
			else if(strcmp(cmd, "manage") == 0) {
				run_manage();
				break;
			}
		}
	}
	return;
}
