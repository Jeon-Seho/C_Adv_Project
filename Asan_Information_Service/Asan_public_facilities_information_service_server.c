#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include <time.h>

#define MAX_BUF 8192 //최대 버퍼 크기
#define MAX_CLNT 256 //최대 접속 가능 클라이언트
#define MAX_STR 100 //최대 문자열 길이
#define MAX_LEN 20 //최대 ID, PW 길이
#define FILE_CNT 6 //파일 개수
#define PAGE 5 //페이지 단위

//연결리스트 구조체
typedef struct INFO {
	int num;				//연번
	char name[MAX_STR];		//시설명
	char address[MAX_STR];	//시설 주소
	char note[MAX_LEN];		//비고
	struct INFO* prev;		//자기 참조 구조체
	struct INFO* next;		//자기 참조 구조체
} INFO;

typedef struct USER {
	char id[MAX_LEN];	//ID
	char pw[MAX_LEN];	//PW
	int permission;		//권한
	struct USER* next;	//자기 참조 구조체
} USER;

typedef struct REVIEW {
	int division;				//구분
	int num;					//연번
	char text[MAX_STR * 10];	//시설 후기 내용
	struct REVIEW* next;		//자기 참조 구조체
} REVIEW;

unsigned WINAPI HandleClient(void* arg); //클라이언트 관리 스레드 함수
void ErrorHandling(char* msg); //오류 상황 처리 함수

void InfoFileRead(); //공공시설 정보 파일 저장 함수
void UserFileRead(); //사용자 정보 파일 저장 함수
void ReviewFileRead(); //시설 후기 정보 파일 저장 함수
void InfoFileUpdate(int num); //공공시설 정보 파일 업데이트 함수
void UserFileUpdate(); //사용자 정보 파일 업데이트 함수
void ReviewFileUpdate(); //시설 후기 정보 파일 업데이트 함수
void InfoFree(); //공공시설 정보 리스트 메모리 해제 함수
void UserFree(); //사용자 정보 리스트 메모리 해제 함수
void ReviewFree(); //시설 후기 정보 리스트 메모리 해제 함수

void SignUpProcess(SOCKET clientSock, char* ptr); //회원가입 함수
int LoginProcess(SOCKET clientSock, char* ptr); //로그인 함수

void ReviewWrite(SOCKET clientSock, INFO* select_facility, int division, char* ptr); //시설 후기 작성 함수
void InfoEdit(SOCKET clientSock, INFO* select_facility, int division, char* ptr); //시설 정보 수정 함수
void UserEdit(SOCKET clientSock, USER* current_user, char* ptr); //사용자 정보 수정 함수
void UserDelete(SOCKET clientSock, char* ptr); //사용자 정보 삭제 함수

//리스트 헤드 주소
INFO* disabled_head = NULL;
INFO* local_head = NULL;
INFO* old_head = NULL;
INFO* park_head = NULL;
INFO* rest_head = NULL;
INFO* hall_head = NULL;
USER* user_head = NULL;
REVIEW* review_head = NULL;

//헤드 주소를 저장하는 이중 포인터 배열
INFO** all_heads[FILE_CNT] = {&disabled_head, &local_head, &old_head, &park_head, &rest_head, &hall_head};

//파일명을 저장하는 문자열 포인터 배열
char* all_files[FILE_CNT] = {
		"Asan_disabled_facility.csv",
		"Asan_local_government.csv",
		"Asan_old_facility.csv",
		"Asan_parking_lot.csv",
		"Asan_rest_area.csv",
		"Asan_town_hall.csv",
};

//전역 변수
SOCKET serverSock;
SOCKET clientSocks[MAX_CLNT]; //클라이언트 소켓 보관 배열
HANDLE hMutex; //뮤텍스
int clientCount = 0; //클라이언트 개수 변수
int serverExit = 0;

int main() {
	WSADATA wsaData;
	SOCKET clientSock;
	SOCKADDR_IN serverAddr, clientAddr;
	int clientAddrSize;
	HANDLE hThread, hInputThread;

	char port[100] = "55555";

	//파일 읽기 및 저장
	InfoFileRead(); //공공시설 정보 저장 함수 호출
	UserFileRead(); //사용자 정보 저장 함수 호출
	ReviewFileRead(); //시설 후기 정보 저장 함수 호출

	//소켓 사용 선언
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup() error!");
	}
	
	// 뮤텍스 생성
	hMutex = CreateMutex(NULL, FALSE, NULL);

	//소켓 생성
	serverSock = socket(PF_INET, SOCK_STREAM, 0);

	//서버 주소 정보 저장
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(atoi(port));

	//소켓 배치 및 준비
	if (bind(serverSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		ErrorHandling("bind() error");
	}
	if (listen(serverSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error");
	}
		
	printf("server start!\n");

	//클라이언트 스레드 생성
	while (serverExit == 0) {
		clientAddrSize = sizeof(clientAddr);
		clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &clientAddrSize); //서버에게 전달된 클라이언트 소켓을 clientSock에 전달

		WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
		clientSocks[clientCount++] = clientSock; //클라이언트 소켓배열에 방금 가져온 소켓 주소를 전달, 클라이언트 개수 1 증가
		ReleaseMutex(hMutex); //뮤텍스 중지

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (void*)&clientSock, 0, NULL); //HandleClient 쓰레드 실행, clientSock을 매개변수로 전달
		printf("Connected Client IP : %s\n", inet_ntoa(clientAddr.sin_addr)); //접속한 클라이언트의 IP 출력
	}

	//소켓 종료 처리
	closesocket(serverSock);
	WSACleanup();

	//리스트 메모리 해제
	InfoFree();
	UserFree();
	ReviewFree();

	//프로그램 종료
	return 0;
}

//클라이언트 관리 스레드 함수
unsigned WINAPI HandleClient(void* arg) {
	SOCKET clientSock = *((SOCKET*)arg); //매개변수로 받은 클라이언트 소켓을 전달
	char recvMsg[MAX_BUF], sendMsg[MAX_BUF]; //수신 버퍼 및 송신 버퍼 변수
	int msgLen = 0; //recv한 메시지 버퍼의 크기를 저장하는 변수

	//로그인, 비회원, 회원가입 반복문
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) { //클라이언트로부터 메시지 수신
		recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

		char* ptr = strtok(recvMsg, "/");

		if (strcmp(ptr, "Login") == 0) {
			ptr = strtok(NULL, "/");

			//회원가입
			if (strcmp(ptr, "new") == 0) {
				SignUpProcess(clientSock, ptr);
			}
			//로그인
			else if (strcmp(ptr, "member") == 0) {
				if (LoginProcess(clientSock, ptr)) {
					break;
				}
			}
			//비회원
			else if (strcmp(ptr, "guest") == 0) {
				sprintf(sendMsg, "Login/guest/success");
				send(clientSock, sendMsg, strlen(sendMsg), 0);
				break;
			}
		}
		//예외 처리
		else {
			
		}
	}

	//기능 선택 반복문
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) { //클라이언트로부터 메시지 수신
		recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

		char* ptr = strtok(recvMsg, "/");

		//공공시설 정보 조회
		if (strcmp(ptr, "Info") == 0) {
			int division;
			char facilityName[MAX_STR];
			char facilityAddress[MAX_STR];
			
			ptr = strtok(NULL, "/");

			if (strcmp(ptr, "division") == 0) {
				ptr = strtok(NULL, "/");
				division = atoi(ptr) - 1;
			}

			WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
			INFO* current_info = *(all_heads[division]);
			ReleaseMutex(hMutex); //뮤텍스 중지

			int startCheck = 0, endCheck = 0;
			while (1) {
				WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
				//5개 단위로 시설명 목록 출력
				sendMsg[0] = '\0'; //sendMsg 초기화
				for (int i = 0; i < PAGE; i++) {
					char tmpMsg[1024];

					sprintf(tmpMsg, "%d. %s ", current_info->num, current_info->name);
					strcat(sendMsg, tmpMsg); //시설 목록 저장

					//마지막 노드인지 확인
					if (current_info->next == NULL) {
						endCheck = 1;
						break;
					}
					else {
						current_info = current_info->next;
					}
				}
				ReleaseMutex(hMutex); //뮤텍스 중지
				
				if (startCheck) {
					strcat(sendMsg, "/start"); //처음 페이지를 의미하는 플래그 추가
					startCheck = 0;
				}
				else if (endCheck) {
					strcat(sendMsg, "/end"); //마지막 페이지를 의미하는 플래그 추가
					endCheck = 0;
				}
				else {
					strcat(sendMsg, "/normal");
				}
				send(clientSock, sendMsg, strlen(sendMsg), 0);

				//이전, 이후, 선택, 검색
				msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //클라이언트로부터 메시지 수신
				recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

				ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "Info") == 0) {
					ptr = strtok(NULL, "/");

					//이전 페이지
					if (strcmp(ptr, "prev") == 0) {
						WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
						//주소를 10칸 이전으로 이동
						for (int i = 0; i < PAGE * 2; i++) {
							//첫번째 노드인지 확인
							if (current_info == *(all_heads[division])) {
								startCheck = 1;
								break;
							}
							else {
								current_info = current_info->prev;
							}
						}
						ReleaseMutex(hMutex); //뮤텍스 중지
					}
					//다음 페이지
					else if (strcmp(ptr, "next") == 0) {
						continue;
					}
					//주소 검색
					else if (strcmp(ptr, "search") == 0) {
						ptr = strtok(NULL, "/");
						strcpy(facilityAddress, ptr);

						//strstr 함수를 사용하여 해당 division에 동일한 주소를 가진 노드가 있는지 확인
						sendMsg[0] = '\0'; //sendMsg 초기화
						current_info = *(all_heads[division]);
						while (current_info != NULL) {
							if (strstr(current_info->address, facilityAddress) != NULL) {
								char tmpMsg[1024];

								sprintf(tmpMsg, "[%d. %s]\n", current_info->num, current_info->name);
								strcat(sendMsg, tmpMsg);
							}

							current_info = current_info->next;
						}

						//만약 노드가 하나도 없다면 안내 송신
						if (sendMsg[0] == '\0') {
							sprintf(sendMsg, "Info/search/fail");
							send(clientSock, sendMsg, strlen(sendMsg), 0);

							continue;
						}
						//검색 결과를 송신
						else {
							send(clientSock, sendMsg, strlen(sendMsg), 0);

							//시설명을 select 받음
							msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //클라이언트로부터 메시지 수신
							recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

							ptr = strtok(recvMsg, "/");
							if (strcmp(ptr, "Info") == 0) {
								ptr = strtok(NULL, "/");

								if (strcmp(ptr, "select") == 0) {
									ptr = strtok(NULL, "/");
									strcpy(facilityName, ptr); //입력한 시설명을 복사

									break;
								}
							}
						}
					}
					//시설명 선택
					else if (strcmp(ptr, "select") == 0) {
						ptr = strtok(NULL, "/");
						
						strcpy(facilityName, ptr); //입력한 시설명을 복사
						break;
					}
					//예외 처리
					else {

					}
				}
			}

			WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
			//선택한 시설명 탐색
			INFO* select_facility = *(all_heads[division]);
			while (select_facility != NULL) {
				if (strcmp(select_facility->name, facilityName) == 0) {
					break;
				}
				select_facility = select_facility->next;
			}
			ReleaseMutex(hMutex); //뮤텍스 중지

			//입력한 시설명이 리스트에 존재하지 않으면
			if (select_facility == NULL) {
				sprintf(sendMsg, "Info/select/fail");
				send(clientSock, sendMsg, strlen(sendMsg), 0);
				continue;
			}

			//선택한 시설의 상세 정보 송신
			sprintf(sendMsg, "Info/select/시설명: %s | 주소: %s | 비고: %s", select_facility->name, select_facility->address, select_facility->note);
			send(clientSock, sendMsg, strlen(sendMsg), 0);

			WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
			//후기 정보 탐색 후 송신
			sendMsg[0] = '\0'; //sendMsg 초기화
			REVIEW* current_review = review_head;
			while (current_review != NULL) {
				if (current_review->division == division && current_review->num == select_facility->num) {
					char tmpMsg[1024];

					sprintf(tmpMsg, "%s\n", current_review->text);
					strcat(sendMsg, tmpMsg);
				}
				current_review = current_review->next;
			}
			ReleaseMutex(hMutex); //뮤텍스 중지

			//리뷰데이터가 없는 경우
			if (sendMsg[0] == '\0') {
				sprintf(sendMsg, "후기 정보가 없습니다!\n");
			}
			send(clientSock, sendMsg, strlen(sendMsg), 0); //서버로 송신

			//기능 선택(1. 후기 작성, 2. 시설 정보 수정, 3. 종료)
			msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //클라이언트로부터 메시지 수신
			recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

			ptr = strtok(recvMsg, "/");
			
			if (strcmp(ptr, "Info") == 0) {
				ptr = strtok(NULL, "/");

				//시설 후기 작성
				if (strcmp(ptr, "review") == 0) {
					ReviewWrite(clientSock, select_facility, division, ptr);
				}
				//시설 정보 수정
				else if (strcmp(ptr, "edit") == 0) {
					InfoEdit(clientSock, select_facility, division, ptr);
				}
				//종료
				else if (strcmp(ptr, "end") == 0) {
					continue;
				}
				//예외 처리
				else {

				}
			}
		}
		//사용자 정보 관리
		else if (strcmp(ptr, "User") == 0) {
			char id[MAX_LEN], pw[MAX_LEN];
			
			ptr = strtok(NULL, "/");

			//비밀번호 일치 여부 확인
			if (strcmp(ptr, "checkpw") == 0) {
				ptr = strtok(NULL, "/");
				strcpy(id, ptr);

				ptr = strtok(NULL, "/");
				strcpy(pw, ptr);
			}

			//리스트에서 해당 ID를 탐색
			WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
			USER* tmp = user_head;
			while (tmp != NULL) {
				if (strcmp(tmp->id, id) == 0) {
					break;
				}
				tmp = tmp->next;
			}

			//해당 ID의 PW와 클라이언트가 입력한 PW가 동일한지 확인
			if (strcmp(tmp->pw, pw) == 0) { //동일하다면 성공 신호 송신
				
				ReleaseMutex(hMutex); //뮤텍스 중지
				sprintf(sendMsg, "User/checkpw/success");
				send(clientSock, sendMsg, strlen(sendMsg), 0);

				msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //클라이언트로부터 메시지 수신
				recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

				ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "User") == 0) {
					ptr = strtok(NULL, "/");
					
					//사용자 정보 수정
					if (strcmp(ptr, "edit") == 0) {
						UserEdit(clientSock, tmp, ptr);
					}
					//사용자 정보 삭제
					else if (strcmp(ptr, "delete") == 0) {
						UserDelete(clientSock, ptr);
					}
					//예외 처리
					else {

					}
				}
				//예외 처리
				else {

				}
			}
			//동일하지 않다면 실패 신호 송신
			else {
				ReleaseMutex(hMutex); //뮤텍스 중지
				sprintf(sendMsg, "User/checkpw/fail");
				send(clientSock, sendMsg, strlen(sendMsg), 0);
			}
		}
		//프로그램 종료
		else if (strcmp(ptr, "Exit") == 0) {
			//클라이언트 소켓 삭제
			printf("client left the program\n");
			WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
			for (int i = 0; i < clientCount; i++) { //클라이언트의 개수만큼 반복
				if (clientSock == clientSocks[i]) { //만약 현재 clientSock값이 배열의 값과 같다면
					while (i++ < clientCount - 1) //클라이언트 개수 만큼
						clientSocks[i] = clientSocks[i + 1]; //앞으로 당김
					break;
				}
			}
			clientCount--; //클라이언트 개수 하나 감소
			ReleaseMutex(hMutex); //뮤텍스 중지
			closesocket(clientSock); //클라이언트 소켓 종료
			break;
		}
		//예외 처리
		else {
			
		}
	}

	return 0;
}

//공공시설 정보 파일 저장 함수
void InfoFileRead() {
	FILE* fp;
	char tmpData[MAX_STR * 5]; //임시 문자열 저장 변수

	for (int i = 0; i < FILE_CNT; i++) {
		fp = fopen(all_files[i], "r");
		if (fp == NULL) {
			ErrorHandling("공공시설 정보 파일 탐색에 실패하였습니다!");
		}

		fgets(tmpData, sizeof(tmpData), fp); //첫 줄 건너뜀

		//공공시설 정보 저장
		while (fgets(tmpData, sizeof(tmpData), fp) != NULL) {
			tmpData[strcspn(tmpData, "\n")] = '\0'; //개행문자 제거

			INFO* new_node = (INFO*)malloc(sizeof(INFO));
			new_node->prev = NULL;
			new_node->next = NULL;
			
			//정보 저장
			char* ptr = strtok(tmpData, ",");
			new_node->num = atoi(ptr); //연번 저장
			ptr = strtok(NULL, ",");
			strcpy(new_node->name, ptr); //시설명 저장
			ptr = strtok(NULL, ",");
			strcpy(new_node->address, ptr); //주소 저장
			ptr = strtok(NULL, ",");
			strcpy(new_node->note, ptr); //비고 저장
			
			//리스트의 가장 마지막에 삽입
			if (*(all_heads[i]) == NULL) {
				*(all_heads[i]) = new_node;
			}
			else {
				INFO* tmp = *(all_heads[i]);
				while (tmp->next != NULL) {
					tmp = tmp->next;
				}

				new_node->prev = tmp;
				tmp->next = new_node;
			}
		}

		fclose(fp);
	}
}

//사용자 정보 파일 저장 함수
void UserFileRead() {
	FILE* fp;
	char tmpData[MAX_STR * 5]; //임시 문자열 저장 변수

	fp = fopen("users.txt", "r"); //사용자 정보 파일을 읽기 모드로 엶
	if (fp == NULL) { //파일을 찾지 못하여 NULL을 반환한 경우
		ErrorHandling("사용자 정보 파일 탐색에 실패하였습니다!");
	}

	//사용자 정보 저장
	while (fgets(tmpData, sizeof(tmpData), fp) != NULL) {
		tmpData[strcspn(tmpData, "\n")] = '\0'; //개행문자 제거

		USER* new_node = (USER*)malloc(sizeof(USER));
		new_node->next = NULL;

		char* ptr = strtok(tmpData, "\t");
		strcpy(new_node->id, ptr); //ID 저장
		ptr = strtok(NULL, "\t");
		strcpy(new_node->pw, ptr); //PW 저장
		ptr = strtok(NULL, "\t");
		new_node->permission = atoi(ptr); //권한 저장

		//리스트의 가장 마지막에 삽입
		if (user_head == NULL) {
			user_head = new_node;
		}
		else {
			USER* tmp = user_head;
			while (tmp->next != NULL) {
				tmp = tmp->next;
			}

			tmp->next = new_node;
		}
	}
	fclose(fp);
}

//시설 후기 정보 파일 저장 함수
void ReviewFileRead() {
	FILE* fp;
	char tmpData[MAX_STR * 5]; //임시 문자열 저장 변수

	fp = fopen("reviews.txt", "r"); //시설 후기 정보 파일을 읽기 모드로 엶
	if (fp == NULL) { //파일을 찾지 못하여 NULL을 반환한 경우
		ErrorHandling("시설 후기 정보 파일 탐색에 실패하였습니다!");
	}

	//시설 후기 정보 저장
	while (fgets(tmpData, sizeof(tmpData), fp) != NULL) {
		tmpData[strcspn(tmpData, "\n")] = '\0'; //개행문자 제거

		REVIEW* new_node = (REVIEW*)malloc(sizeof(REVIEW));
		new_node->next = NULL;

		char* ptr = strtok(tmpData, "\t");

		new_node->division = atoi(ptr); //구분 저장
		ptr = strtok(NULL, "\t");
		new_node->num = atoi(ptr); //연번 저장
		ptr = strtok(NULL, "\t");
		strcpy(new_node->text, ptr); //후기 저장

		//리스트의 가장 마지막에 삽입
		if (review_head == NULL) {
			review_head = new_node;
		}
		else {
			REVIEW* tmp = review_head;
			while (tmp->next != NULL) {
				tmp = tmp->next;
			}

			tmp->next = new_node;
		}
	}
	fclose(fp);
}

//공공시설 정보 파일 업데이트 함수
void InfoFileUpdate(int num) {
	FILE* fp;

	//파일을 쓰기 모드로 엶
	fp = fopen(all_files[num], "w");
	if (fp == NULL) {
		ErrorHandling("파일 탐색에 실패하였습니다!");
	}

	//반복문으로 프로그램이 가지고 있는 구조체의 정보를 파일에 덮어씀
	INFO* tmp = *(all_heads[num]);
	fprintf(fp, "연번,시설명,주소,비고\n");
	while(tmp != NULL) {
		fprintf(fp, "%d,%s,%s,%s\n", tmp->num, tmp->name, tmp->address, tmp->note);
		tmp = tmp->next;
	}

	fclose(fp); //파일을 닫음
}

//사용자 정보 파일 업데이트 함수
void UserFileUpdate() {
	FILE* fp;

	//파일을 쓰기 모드로 엶
	fp = fopen("users.txt", "w");
	if (fp == NULL) {
		ErrorHandling("파일 탐색에 실패하였습니다!");
	}

	//반복문으로 프로그램이 가지고 있는 구조체의 정보를 파일에 덮어씀
	USER* tmp = user_head;
	while (tmp != NULL) {
		fprintf(fp, "%s\t%s\t%d\n", tmp->id, tmp->pw, tmp->permission);
		tmp = tmp->next;
	}

	fclose(fp); //파일을 닫음
}

//시설 후기 정보 파일 업데이트 함수
void ReviewFileUpdate() {
	FILE* fp;

	//파일을 쓰기 모드로 엶
	fp = fopen("reviews.txt", "w");
	if (fp == NULL) {
		ErrorHandling("파일 탐색에 실패하였습니다!");
	}

	//반복문으로 프로그램이 가지고 있는 구조체의 정보를 파일에 덮어씀
	REVIEW* tmp = review_head;
	while (tmp != NULL) {
		fprintf(fp, "%d\t%d\t%s\n", tmp->division, tmp->num, tmp->text);
		tmp = tmp->next;
	}

	fclose(fp); //파일을 닫음
}

//회원가입 함수
void SignUpProcess(SOCKET clientSock, char* ptr) {
	char sendMsg[MAX_BUF]; //송신 버퍼 변수
	char id[MAX_LEN], pw[MAX_LEN];

	ptr = strtok(NULL, "/");
	strcpy(id, ptr); //클라이언트가 입력한 ID 복사
	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); //클라이언트가 입력한 PW 복사

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행

	//ID 중복 검사
	USER* tmp = user_head;
	while (tmp != NULL) {
		if (strcmp(tmp->id, id) == 0) {
			ReleaseMutex(hMutex); //뮤텍스 중지
			sprintf(sendMsg, "Login/new/fail");
			send(clientSock, sendMsg, strlen(sendMsg), 0);
			return;
		}
		tmp = tmp->next;
	}

	//새로운 노드 생성
	USER* new_node = (USER*)malloc(sizeof(USER));

	strcpy(new_node->id, id); //ID 저장
	strcpy(new_node->pw, pw); //PW 저장
	new_node->permission = 0; //권한 초기화
	new_node->next = NULL;

	//리스트의 가장 마지막에 삽입
	if (user_head == NULL) {
		user_head = new_node;
	}
	else {
		tmp = user_head;
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}

		tmp->next = new_node;
	}
	UserFileUpdate(); //사용자 정보 파일 업데이트
	ReleaseMutex(hMutex); //뮤텍스 중지

	//회원가입 성공 신호 송신
	sprintf(sendMsg, "Login/new/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
}

//로그인 함수
int LoginProcess(SOCKET clientSock, char* ptr) {
	char sendMsg[MAX_BUF]; //송신 버퍼 변수
	char id[MAX_LEN], pw[MAX_LEN];
	int loginCheck = 0; //로그인 성공 여부를 확인하는 변수

	ptr = strtok(NULL, "/");
	strcpy(id, ptr); //클라이언트가 입력한 ID 복사

	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); //클라이언트가 입력한 PW 복사

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	//클라이언트가 입력한 ID와 PW가 사용자 정보 연결리스트에 존재하는지 순회하며 탐색 
	USER* tmp = user_head;
	while (tmp != NULL) {
		//ID와 PW가 모두 일치할 때
		if (strcmp(tmp->id, id) == 0 && strcmp(tmp->pw, pw) == 0) {
			loginCheck = 1; //로그인 성공 체크
			break;
		}
		tmp = tmp->next;
	}
	ReleaseMutex(hMutex); //뮤텍스 중지

	if (loginCheck) {
		sprintf(sendMsg, "Login/member/success/%d", tmp->permission);
		send(clientSock, sendMsg, strlen(sendMsg), 0); //로그인 성공 신호 send
		return 1;
	}
	else {
		sprintf(sendMsg, "Login/member/fail");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //로그인 실패 신호 send
		return 0;
	}
}

//시설 후기 작성 함수
void ReviewWrite(SOCKET clientSock, INFO* select_facility, int division, char* ptr) {
	REVIEW* new_node = (REVIEW*)malloc(sizeof(REVIEW));
	time_t rawtime;
	struct tm* timeInfo;

	char sendMsg[MAX_BUF]; //송신 버퍼 변수
	char id[MAX_LEN];
	char reviewData[MAX_STR * 10];
	char timeData[MAX_STR]; // 시간을 저장하는 문자열

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	ptr = strtok(NULL, "/");
	strcpy(id, ptr);
	ptr = strtok(NULL, "/");
	strcpy(reviewData, ptr);

	new_node->division = division;
	new_node->num = select_facility->num;
	new_node->next = NULL;

	time(&rawtime); //현재 시간 저장
	timeInfo = localtime(&rawtime); //시간 정보를 로컬 시간대에 맞게 변환
	strftime(timeData, sizeof(timeData), "%y/%m/%d %H:%M", timeInfo); //원하는 형식으로 문자열에 저장
	sprintf(new_node->text, "[%s][%s][%s]", id, timeData, reviewData);

	//리스트의 마지막에 삽입
	REVIEW* tmp = review_head;
	if (review_head == NULL) {
		review_head = new_node;
	}
	else {
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}

		tmp->next = new_node;
	}

	ReviewFileUpdate();
	ReleaseMutex(hMutex); //뮤텍스 중지

	sprintf(sendMsg, "Info/review/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
}

//시설 정보 수정 함수
void InfoEdit(SOCKET clientSock, INFO* select_facility, int division, char* ptr) {
	char sendMsg[MAX_BUF]; //송신 버퍼 변수
	
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행

	//시설명 수정
	if (strcmp(ptr, "name") == 0) {
		ptr = strtok(NULL, "/");
		strcpy(select_facility->name, ptr);
	}
	//시설 주소 수정
	else if (strcmp(ptr, "address") == 0) {
		ptr = strtok(NULL, "/");
		strcpy(select_facility->address, ptr);
	}
	//예외 처리
	else{
		
	}

	InfoFileUpdate(division);
	ReleaseMutex(hMutex); //뮤텍스 중지

	sprintf(sendMsg, "Info/edit/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
}

//사용자 정보 수정 함수
void UserEdit(SOCKET clientSock, USER* current_user, char* ptr) {
	char recvMsg[MAX_BUF], sendMsg[MAX_BUF]; //수신 버퍼 및 송신 버퍼 변수
	int msgLen = 0; //recv한 메시지 버퍼의 크기를 저장하는 변수
	
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	strcpy(current_user->pw, ptr); //기존 비밀번호를 새로운 비밀번호로 교체
	UserFileUpdate();
	ReleaseMutex(hMutex); //뮤텍스 중지

	sprintf(sendMsg, "User/edit/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
	return;
}

//사용자 정보 삭제 함수
void UserDelete(SOCKET clientSock, char* ptr) {
	char sendMsg[MAX_BUF]; //송신 버퍼 변수
	char id[MAX_LEN];

	ptr = strtok(NULL, "/");
	strcpy(id, ptr);

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행

	//리스트가 비어있을 경우
	if (user_head == NULL) {
		sprintf(sendMsg, "User/delete/fail"); //실패 신호 송신
		send(clientSock, sendMsg, strlen(sendMsg), 0);
		ReleaseMutex(hMutex); //뮤텍스 중지
		return;
	}

	//리스트의 헤드가 삭제 대상인 경우
	if (strcmp(user_head->id, id) == 0) {
		//해당 노드를 리스트에서 삭제
		USER* current = user_head;
		user_head = user_head->next;
		free(current);

		sprintf(sendMsg, "User/delete/success"); //성공 신호 송신
		send(clientSock, sendMsg, strlen(sendMsg), 0);
		UserFileUpdate();
		ReleaseMutex(hMutex); //뮤텍스 중지
		return;
	}

	//리스트에서 해당 ID를 찾음
	USER* tmp = user_head;
	while (tmp->next != NULL) {
		if (strcmp(tmp->next->id, id) == 0) {
			//해당 노드를 리스트에서 삭제
			USER* current = tmp->next;
			tmp->next = tmp->next->next;
			free(current);
			
			sprintf(sendMsg, "User/delete/success"); //성공 신호 송신
			send(clientSock, sendMsg, strlen(sendMsg), 0);
			UserFileUpdate();
			ReleaseMutex(hMutex); //뮤텍스 중지
			return;
		}
		tmp = tmp->next;
	}

	ReleaseMutex(hMutex); //뮤텍스 중지
}

//공공시설 정보 리스트 메모리 해제 함수
void InfoFree() {
	for (int i = 0; i < FILE_CNT; i++) {
		INFO* current = *(all_heads[i]);

		while (current != NULL) {
			INFO* next_node = current->next;
			free(current);
			current = next_node;
		}
		*(all_heads[i]) = NULL;
	}
}

//사용자 정보 리스트 메모리 해제 함수
void UserFree() {
	USER* current = user_head;

	while (current != NULL) {
		USER* next_node = current->next;
		free(current);
		current = next_node;
	}
	user_head = NULL;
}

//시설 후기 정보 리스트 메모리 해제 함수
void ReviewFree() {
	REVIEW* current = review_head;

	while (current != NULL) {
		REVIEW* next_node = current->next;
		free(current);
		current = next_node;
	}
	review_head = NULL;
}

//오류 상황 처리 함수
void ErrorHandling(char* msg) {
	fputs(msg, stderr); //매개변수로 받은 메시지를 출력
	fputc('\n', stderr); //개행 문자 출력
	exit(1); //프로그램 종료
}