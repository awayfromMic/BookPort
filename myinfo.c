#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"

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
	RecordEntry* user_record = (RecordEntry*)malloc(2 * lend_return_count * sizeof(RecordEntry)); //각 파일 linecount, load필요
	int user_record_count = 0;
	for (int i = 0; i < lend_return_count; i++) {
		if (strcmp(current_user.studentId, Lend_Return[i].userid) == 0) {	
			strcpy(user_record[user_record_count].date, Lend_Return[i].borrowDate);
			strcpy(user_record[user_record_count].type, "borrow");
			strcpy(user_record[user_record_count].bid, Lend_Return[i].bookBid);
			user_record_count++;
			if (Lend_Return[i].returnDate[0] != '\0') {
				strcpy(user_record[user_record_count].date, Lend_Return[i].returnDate);
				strcpy(user_record[user_record_count].type, "return");
				strcpy(user_record[user_record_count].bid, Lend_Return[i].bookBid);
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

	while (1) {
		printf("BookPort: My info - Enter command>");
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

				for (int j = 0; j < bookcount; j++) {
					if (strcmp(Book[j].bookBid, current_user.lentBids[i]) == 0) {
						printf("Title: %s\n", Book[j].title);
						printf("Author: %s\n", Book[j].author);
						printf("BID: %s\n", Book[j].bid);
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
	else {
		run_login();
	}
	return;
}

#define MIN_PW_LEN 5
#define MAX_PW_LEN 20

// 전역 로그인 상태 변수들. run_login() 시점에 설정되어 있어야 함
extern int is_logged_in;
extern User current_user; 

extern void run_verify(); // 무결성 검사
extern void run_logout(); // 로그아웃

// 입력이 버퍼를 초과하였을 때 flush하는 기능
void flush_stdin() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

// 공백류 처리 (개행은 유지)
int is_custom_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r');
}
void trim(char* str) {
    while (is_custom_whitespace(*str)) memmove(str, str + 1, strlen(str));
    int len = strlen(str);
    while (len > 0 && is_custom_whitespace(str[len - 1])) str[--len] = '\0';
}

// 비밀번호 문법 검사. 
// 회원가입할때 사용한 함수 있으면 그거 써도 될듯 싶다.
// 반환값 1 = 유효, 0 = 에러 (에러 메시지 출력 포함)
// 이거 login.c에 들어있을 것 같은데 여기서 그냥 썼음.
int validate_password(const char *pw) {
    int len = strlen(pw);

    // 1. 비어 있는 문자열
    if (len == 0) {
        printf(".!! Error: Password cannot be an empty string.\n");
        return 0;
    }

    // 2. 공백 문자만 존재하는지
    int only_whitespace = 1;
    for (int i = 0; i < len; i++) {
        if (!isspace(pw[i])) {
            only_whitespace = 0;
            break;
        }
    }
    if (only_whitespace) {
        printf(".!! Error: Password cannot consist of only whitespace characters.\n");
        return 0;
    }

    // 3. 탭 문자가 유효 문자 사이에 있는 경우 (↹ = \t)
    int first_valid = -1, last_valid = -1;
    for (int i = 0; i < len; i++) {
        if (!isspace(pw[i])) {
            if (first_valid == -1) first_valid = i;
            last_valid = i;
        }
    }
    for (int i = first_valid + 1; i < last_valid; i++) {
        if (pw[i] == '\t') {
            printf(".!! Error: A tab character cannot be placed between the first and last valid characters of the password.\n");
            return 0;
        }
    }

    // 4. 길이 제한
    if (len < MIN_PW_LEN || len > MAX_PW_LEN) {
        printf(".!! Error: Password must be 5 to 20 characters long.\n");
        return 0;
    }

    // 5. 영문자와 숫자 하나 이상 포함 여부
    int has_alpha = 0, has_digit = 0;
    for (int i = 0; i < len; i++) {
        if (isalpha(pw[i])) has_alpha = 1;
        if (isdigit(pw[i])) has_digit = 1;
    }
    if (!has_alpha || !has_digit) {
        printf(".!! Error: Password must contain at least one letter and at least one digit.\n");
        return 0;
    }

    // 6. 동일 문자 5회 이상 연속 사용 금지
    int counts[128] = {0};
    for (int i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)pw[i];
        counts[ch]++;
        if (counts[ch] >= 5) {
            printf(".!! Error: The password cannot contain 5 or more of the same letter, digit, or special character.\n");
            return 0;
        }
    }

    // 모두 통과
    return 1;
}

// ######## 비밀번호 변경 ######## //
void change_password() {

    char new_pw[100];

    while (1) {
        printf("BookPort: Enter your new password > ");
        if (!fgets(new_pw, sizeof(new_pw), stdin)) {
            flush_stdin();
            continue;
        }
        if (strchr(new_pw, '\n') == NULL) flush_stdin();
        new_pw[strcspn(new_pw, "\n")] = '\0';

        // 이전 비밀번호와 동일한 경우
        if (strcmp(new_pw, current_user.password) == 0) {
            printf(".!! Error: Password is the same as before.\n");
            continue;
        }

        // 유효성 검사 실패 시 메시지 출력 후 반복
        if (!validate_password(new_pw)) {
            continue;
        }

        // 유효한 비밀번호라면 루프 종료
        break;
    }

    // ===== 연결 리스트를 통한 파일 수정 =====

    typedef struct Node {
        char line[512];
        char studentId[MAX_ID];
        struct Node* next;
    } Node;

    FILE* fp = fopen(USER_FILE, "r");
    if (!fp) return;

    Node *head = NULL, *tail = NULL;
    char buf[512];      // 임의로 512자로 설정했음
    while (fgets(buf, sizeof(buf), fp)) {
        Node* node = malloc(sizeof(Node));
        strcpy(node->line, buf);
        node->next = NULL;

        char tmp[512];
        strcpy(tmp, buf);
        strtok(tmp, ",");                       // name
        char* sid = strtok(NULL, ",");          // id
        strncpy(node->studentId, sid, MAX_ID);

        if (!head) head = node;
        else tail->next = node;
        tail = node;
    }
    fclose(fp);

    // 새 비밀번호를 반영해서 다시 파일에 쓰기
    fp = fopen(USER_FILE, "w");
    Node* cur = head;
    while (cur) {
        // 비밀번호 변경된 줄 작성
        if (strcmp(cur->studentId, current_user.studentId) == 0) {
            fprintf(fp, "%s,%s,%s,", current_user.name, current_user.studentId, new_pw);
            for (int i = 0; i < 5; i++) {
                fprintf(fp, "%s", current_user.lentBids[i]);
                if (i < 4) fprintf(fp, ";");
            }
            fprintf(fp, ",%d\n", current_user.lendAvailable);
            // current_user 비밀번호도 갱신
            strcpy(current_user.password, new_pw);
        } else {
            fputs(cur->line, fp);
        }
        cur = cur->next;
    }
    fclose(fp);

    // 메모리 해제
    while (head) {
        Node* temp = head;
        head = head->next;
        free(temp);
    }

    printf("\u2026 Your password successfully changed\n");
}

// ######## 회원 탈퇴 ######## //
void withdraw_user() {
    printf("BookPort: Do you want to withdraw?(.../No) > ");
    char input[5];  // 충분한 입력 버퍼 (e.g., "No\n\0")

    int flushed = 0;
    if (!fgets(input, sizeof(input), stdin)) {
        // 입력 실패 (너무 김 등) → 탈퇴 강행, flush 필요
        flushed = 1;
    } else {
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "No") == 0) {
            return;
        }

        // 개행을 못 읽은 경우 (입력 길이 초과) → flush 필요
        if (strchr(input, '\n') == NULL) {
            flushed = 1;
        }
    }

    if (flushed) {
        flush_stdin(); // 버퍼 비우기
    }

    // 이제부터 회원 탈퇴 진행
    // 1. 파일 읽어와 연결 리스트로 구성
    typedef struct UserNode {
        char line[512];
        char studentId[MAX_ID];
        struct UserNode* next;
    } UserNode;

    FILE *fp = fopen(USER_FILE, "r");
    if (!fp) {
        printf(".!! Error: Unable to open user data.\n");
        return;
    }

    UserNode *head = NULL, *tail = NULL;
    char buf[512];

    while (fgets(buf, sizeof(buf), fp)) {
        UserNode* node = malloc(sizeof(UserNode));
        if (!node) continue;
        node->next = NULL;
        strcpy(node->line, buf);

        char tmp[512];
        strcpy(tmp, buf);
        strtok(tmp, ",");              // name
        char* sid = strtok(NULL, ","); // studentId
        if (!sid) {
            free(node);
            continue;
        }
        strncpy(node->studentId, sid, MAX_ID);

        if (!head) head = node;
        else tail->next = node;
        tail = node;
    }
    fclose(fp);

    // 2. 탈퇴 대상 제외 후 덮어쓰기
    fp = fopen(USER_FILE, "w");
    if (!fp) {
        printf(".!! Error: Unable to open user data for writing.\n");
        return;
    }

    UserNode* cur = head;
    while (cur) {
        if (strcmp(cur->studentId, current_user.studentId) != 0) {
            fputs(cur->line, fp); // 탈퇴 대상이 아니면 그대로 씀
        }
        cur = cur->next;
    }
    fclose(fp);

    // 3. 메모리 해제
    while (head) {
        UserNode* temp = head;
        head = head->next;
        free(temp);
    }

    // 4. 후처리
    printf("Withdrawal successful.\n");
    run_verify();  // 파일 무결성 검사 이걸로 하는 게 맞나?
    run_logout();  // 로그인 해제
    // run_logout()에서 is_logged_in = 0 초기화 필요!
}
