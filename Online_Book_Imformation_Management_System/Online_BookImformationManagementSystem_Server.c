#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <process.h>

#define MAX_BUF 8192 //최대 버퍼 크기
#define MAX_CLNT 256 //최대 접속 가능 클라이언트
#define MAX_BOOK 700 //최대 도서 권수
#define MAX_USER 20 //최대 사용자 수
#define MAX_STR 100 //최대 문자열 길이
#define MAX_LEN 20 //최대 ID, PW 길이

unsigned WINAPI HandleClient(void* arg); //클라이언트를 관리하는 스레드 함수

int LoginProcess(SOCKET clientSock, char* recvMsg); //로그인 처리 함수
void BookSearch(SOCKET clientSock, char* ptr); //도서 정보 검색 함수
void BookEdit(SOCKET clientSock, int findIndex, char* ptr); //도서 정보 수정 함수
void BookDelete(SOCKET clientSock, int findIndex, char* ptr); //도서 정보 삭제 함수
void BookAdd(SOCKET clientSock, char* ptr); //도서 정보 추가 함수
void BookRanking(SOCKET clientSock, char* ptr); //평점 순 랭킹 서비스 함수
void UserEdit(SOCKET clientSock, char* ptr); //사용자 정보 수정 함수
void UserDelete(SOCKET clientSock, char* ptr); //사용자 정보 삭제 함수
void UserAdd(SOCKET clientSock, char* ptr); //사용자 정보 추가 함수

void BookFileUpdate(); //도서 정보 파일 업데이트 함수
void UserFileUpdate(); //사용자 정보 파일 업데이트 함수

void ErrorHandling(char* msg); //오류 상황 처리 함수

//도서 정보를 저장하는 구조체
typedef struct bookInfo {
	int num; //순번
	char bookTitle[MAX_STR]; //도서명
	char author[MAX_STR]; //저자명
	float rating; //평점
} BOOK;

//사용자 정보를 저장하는 구조체
typedef struct userInfo {
	char id[MAX_LEN]; //ID
	char pw[MAX_LEN]; //PW
} USER;

//전역 변수
SOCKET clientSocks[MAX_CLNT]; //클라이언트 소켓 보관 배열
BOOK books[MAX_BOOK]; //도서 정보 보관 배열
BOOK* sortedBooks[MAX_BOOK]; //정렬된 도서 정보의 주소를 보관하는 배열
USER users[MAX_USER]; //사용자 정보 보관 배열
HANDLE hMutex; //뮤텍스

int clientCount = 0; //클라이언트 개수
int bookCount = 0; //도서 권 수
int userCount = 0; //사용자 명 수

int msgLen = 0; //recv한 메시지 버퍼의 크기를 저장
char recvMsg[MAX_BUF], sendMsg[MAX_BUF];

int main() {
	WSADATA wsaData;
	SOCKET serverSock, clientSock;
	SOCKADDR_IN serverAddr, clientAddr;
	int clientAddrSize;
	HANDLE hThread;

	char port[100] = "55555";
	char bookData[MAX_STR * 2], userData[MAX_LEN * 2];

	//도서 정보 파일 읽기
	FILE* bookfp;
	bookfp = fopen("booklist2.txt", "r"); //도서 정보 파일을 읽기 모드로 엶
	if (bookfp == NULL) { //파일을 찾지 못하여 NULL을 반환한 경우
		ErrorHandling("파일 탐색에 실패하였습니다!");
	}

	//도서 정보 저장
	while (fgets(bookData, sizeof(bookData), bookfp) != NULL) {
		//개행문자 제거
		for (int i = 0; i < sizeof(bookData); i++) {
			if (bookData[i] == '\n') {
				bookData[i] = '\0';
				break;
			}
		}

		char* ptr = strtok(bookData, "\t");
		books[bookCount].num = atoi(ptr); //순번 저장
		ptr = strtok(NULL, "\t");
		strcpy(books[bookCount].bookTitle, ptr); //도서명 저장
		ptr = strtok(NULL, "\t");
		strcpy(books[bookCount].author, ptr); //저자명 저장
		ptr = strtok(NULL, "\t");
		books[bookCount].rating = atof(ptr); //평점 저장

		bookCount++; //도서 권 수를 1 증가
	}
	fclose(bookfp); //파일을 닫음

	//사용자 정보 파일 읽기
	FILE* userfp;
	userfp = fopen("users.txt", "r"); //사용자 정보 파일을 읽기 모드로 엶
	if (userfp == NULL) { //파일을 찾지 못하여 NULL을 반환한 경우
		ErrorHandling("파일 탐색에 실패하였습니다!");
	}

	//사용자 정보 저장
	while (fgets(userData, sizeof(userData), userfp) != NULL) {
		//개행문자 제거
		for (int i = 0; i < sizeof(userData); i++) {
			if (userData[i] == '\n') {
				userData[i] = '\0';
				break;
			}
		}

		char* ptr = strtok(userData, "//");
		strcpy(users[userCount].id, ptr); //ID 저장
		ptr = strtok(NULL, "//");
		strcpy(users[userCount].pw, ptr); //PW 저장

		userCount++; //사용자 명 수를 1 증가
	}
	fclose(userfp); //파일을 닫음

	//포트 입력
	/*printf("Input port number : ");
	gets(port);*/

	//소켓 사용 선언
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

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
	if (bind(serverSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	if (listen(serverSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");

	printf("listening...\n");

	//클라이언트 스레드 생성
	while (1) {
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

	//프로그램 종료
	return 0;
}

unsigned WINAPI HandleClient(void* arg) {
	SOCKET clientSock = *((SOCKET*)arg); //매개변수로 받은 클라이언트 소켓을 전달
	
	//로그인 처리 반복문
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) {
		recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

		//로그인에 성공했을 경우
		if (LoginProcess(clientSock, recvMsg)) {
			sprintf(sendMsg, "LOGIN/1/SUCCESS");
			send(clientSock, sendMsg, strlen(sendMsg), 0); //로그인 성공 신호 send
			break; //로그인 반복문에서 break
		}
		//로그인에 실패했을 경우
		else {
			sprintf(sendMsg, "LOGIN/1/FAIL");
			send(clientSock, sendMsg, strlen(sendMsg), 0); //로그인 실패 신호 send
		}
	}

	//기능 선택 반복문
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) {
		recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입
		char* ptr = strtok(recvMsg, "/");

		//도서 정보 관리 기능
		if (strcmp(ptr, "Book") == 0) {
			ptr = strtok(NULL, "/");

			//도서 정보 검색 기능
			if (strcmp(ptr, "Search") == 0) {
				BookSearch(clientSock, ptr);
			}
			//도서 정보 추가 기능
			else if (strcmp(ptr, "Add") == 0) {
				BookAdd(clientSock, ptr);
			}
			//도서 평점 순 랭킹 서비스 기능
			else if (strcmp(ptr, "Ranking") == 0) {
				BookRanking(clientSock, ptr);
			}
		}
		//사용자 정보 관리 기능
		else if (strcmp(ptr, "User") == 0) {
			ptr = strtok(NULL, "/");

			//사용자 정보 수정 기능
			if (strcmp(ptr, "Edit") == 0) {
				UserEdit(clientSock, ptr);

			}
			//사용자 정보 삭제 기능
			else if (strcmp(ptr, "Delete") == 0) {
				UserDelete(clientSock, ptr);
			}
			//사용자 정보 추가 기능
			else if (strcmp(ptr, "Add") == 0) {
				UserAdd(clientSock, ptr);
			}
		}
		//클라이언트 종료 기능
		else if (strcmp(ptr, "EXIT") == 0) {
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
		else {
			//예외처리
		}
	}
	return 0;
}

//로그인 처리 함수
int LoginProcess(SOCKET clientSock, char* recvMsg) {
	char id[MAX_STR], pw[MAX_STR];
	int loginCheck = 0; //로그인 성공 여부를 확인하는 변수

	//클라이언트가 보낸 신호가 로그인 시도가 아닐 경우
	char* ptr = strtok(recvMsg, "/");
	if (strcmp(ptr, "LOGIN") != 0) {
		printf("error\n");
		return 0;
	}

	ptr = strtok(NULL, "/");
	if (strcmp(ptr, "0") != 0) {
		printf("error\n");
		return 0;
	}

	ptr = strtok(NULL, "/");
	strcpy(id, ptr); // 클라이언트가 입력한 ID 복사
	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); // 클라이언트가 입력한 PW 복사

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	//클라이언트가 입력한 ID와 PW를 사용자 정보를 담고 있는 구조체 멤버와 비교
	for (int i = 0; i < userCount; i++) {
		//ID와 PW가 모두 일치할 때
		if (strcmp(users[i].id, id) == 0 && strcmp(users[i].pw, pw) == 0) {
			loginCheck = 1; //로그인 성공 체크
			break;
		}
	}
	ReleaseMutex(hMutex); //뮤텍스 중지

	if (loginCheck) {
		return 1;
	}
	else {
		return 0;
	}
}

//도서 정보 검색 함수
void BookSearch(SOCKET clientSock, char* ptr) {
	int findIndex = -1; //검색할 도서 정보의 인덱스
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	//도서 권 수 만큼 반복
	for (int i = 0; i < bookCount; i++) {
		//클라이언트가 입력한 도서명과 구조체의 도서명이 같으면
		if (strcmp(ptr, books[i].bookTitle) == 0) {
			findIndex = i; // 검색할 도서 정보 인덱스에 찾은 인덱스 i를 대입
			break;
		}
	}
	ReleaseMutex(hMutex); //뮤텍스 중지

	//동일한 도서명을 찾지 못했다면
	if (findIndex == -1) {
		sprintf(sendMsg, "Book/Search/0");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //검색 실패 신호 send
	}
	//동일한 도서명을 찾았다면
	else {
		sprintf(sendMsg, "Book/Search/1/\n순번: [%d], 제목: [%s], 저자: [%s], 평점: [%.2f]\n", books[findIndex].num, books[findIndex].bookTitle, books[findIndex].author, books[findIndex].rating);
		send(clientSock, sendMsg, strlen(sendMsg), 0); //도서 정보를 send

		msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //클라이언트로부터 도서 수정, 삭제 여부를 recv
		recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입
		char* ptr = strtok(recvMsg, "/");

		if (strcmp(ptr, "Book") == 0) {
			ptr = strtok(NULL, "/");

			if (strcmp(ptr, "Edit") == 0) {
				BookEdit(clientSock, findIndex, ptr); //도서 정보 수정 함수 호출
			}
			else if (strcmp(ptr, "Delete") == 0) {
				BookDelete(clientSock, findIndex, ptr); //도서 정보 삭제 함수 호출
			}
		}
	}
}

//도서 정보 수정 함수
void BookEdit(SOCKET clientSock, int findIndex, char* ptr) {
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행

	//도서명을 수정하는 경우
	if (strcmp(ptr, "1") == 0) {
		ptr = strtok(NULL, "/");

		strcpy(books[findIndex].bookTitle, ptr); //도서명 복사
	}
	//저자명을 수정하는 경우
	else if (strcmp(ptr, "2") == 0) {
		ptr = strtok(NULL, "/");

		strcpy(books[findIndex].author, ptr); //저자명 복사
	}
	//평점을 수정하는 경우
	else if (strcmp(ptr, "3") == 0) {
		ptr = strtok(NULL, "/");

		books[findIndex].rating = atof(ptr); //평점 대입
	}

	ReleaseMutex(hMutex); //뮤텍스 중지
	BookFileUpdate(); //도서 정보 파일 업데이트
}

//도서 정보 삭제 함수
void BookDelete(SOCKET clientSock, int findIndex, char* ptr) {
	ptr = strtok(NULL, "/");

	//도서 삭제를 최종적으로 결정했다면
	if (strcmp(ptr, "1") == 0) {
		WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
		//삭제할 도서의 인덱스부터 마지막 도서까지
		for (int i = findIndex; i < bookCount - 1; i++) {
			//앞으로 한 칸 씩 당김
			strcpy(books[i].bookTitle, books[i + 1].bookTitle);
			strcpy(books[i].author, books[i + 1].author);
			books[i].rating = books[i + 1].rating;
		}

		bookCount--; //도서 권 수를 1 감소
		ReleaseMutex(hMutex); //뮤텍스 중지
		BookFileUpdate(); //도서 정보 파일 업데이트

	}
	//도서 삭제를 도중에 취소했다면
	else if (strcmp(ptr, "0") == 0) {
		return;
	}
}

//도서 정보 추가 함수
void BookAdd(SOCKET clientSock, char* ptr) {
	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	//현재 구조체의 가장 마지막에 새로운 도서를 추가
	books[bookCount].num = bookCount + 1;

	ptr = strtok(NULL, "/");
	strcpy(books[bookCount].bookTitle, ptr); //클라이언트가 입력한 도서명 복사

	ptr = strtok(NULL, "/");
	strcpy(books[bookCount].author, ptr); //클라이언트가 입력한 저자명 복사

	ptr = strtok(NULL, "/");
	books[bookCount].rating = atof(ptr); //클라이언트가 입력한 평점 복사

	bookCount++; //도서 권 수를 1 증가
	ReleaseMutex(hMutex); //뮤텍스 중지

	BookFileUpdate(); //도서 정보 파일 업데이트
}

//평점 순 랭킹 서비스 함수
void BookRanking(SOCKET clientSock, char* ptr) {
	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	for (int i = 0; i < bookCount; i++) {
		sortedBooks[i] = &books[i]; //구조체 포인터 배열에 모든 구조체의 주소를 저장
	}

	//버블 정렬(내림차순)
	for (int i = 0; i < bookCount - 1; i++) {
		for (int j = 0; j < bookCount - i - 1; j++) {
			//현재 주소의 평점보다 다음 주소의 평점이 더 크다면 
			if (sortedBooks[j]->rating < sortedBooks[j + 1]->rating) {

				//swap
				BOOK* tempPtr = sortedBooks[j];
				sortedBooks[j] = sortedBooks[j + 1];
				sortedBooks[j + 1] = tempPtr;
			}
		}
	}

	sendMsg[0] = '\0'; //sendMsg 초기화
	for (int i = 0; i < 30; i++) {
		char tempMsg[1024];

		sprintf(tempMsg, "순번: [%d], 제목: [%s], 저자: [%s], 평점: [%.2f]\n", sortedBooks[i]->num, sortedBooks[i]->bookTitle, sortedBooks[i]->author, sortedBooks[i]->rating);
		strcat(sendMsg, tempMsg); //평점 순으로 정렬한 데이터를 이어붙임
	}
	send(clientSock, sendMsg, strlen(sendMsg), 0); //이어붙인 데이터를 송신
	ReleaseMutex(hMutex); //뮤텍스 중지
}

//사용자 정보 수정 함수
void UserEdit(SOCKET clientSock, char* ptr) {
	char id[MAX_STR], pw[MAX_STR];
	char newPw[MAX_STR];
	int check = 0;

	ptr = strtok(NULL, "/");
	strcpy(id, ptr); //클라이언트가 입력한 ID 복사

	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); //클라이언트가 입력한 PW 복사

	ptr = strtok(NULL, "/");
	strcpy(newPw, ptr); //클라이언트가 입력한 새로운 PW 복사

	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	for (int i = 0; i < userCount; i++) {
		//id와 pw가 일치하다면
		if (strcmp(users[i].id, id) == 0 && strcmp(users[i].pw, pw) == 0) {
			strcpy(users[i].pw, newPw); //pw를 새로운 pw로 복사
			check = 1; //사용자 정보 변경 성공 체크
		}
	}
	ReleaseMutex(hMutex); //뮤텍스 중지
	UserFileUpdate(); //사용자 정보 파일 업데이트

	//사용자 정보 변경에 성공했을 시
	if (check) {
		sprintf(sendMsg, "User/Edit/1");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //사용자 정보 변경 성공 신호 송신
	}
	else {
		sprintf(sendMsg, "User/Edit/0");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //사용자 정보 변경 실패 신호 송신
	}
}

//사용자 정보 삭제 함수
void UserDelete(SOCKET clientSock, char* ptr) {
	char id[MAX_STR], pw[MAX_STR];
	ptr = strtok(NULL, "/");

	//사용자 정보 삭제를 최종적으로 결정했다면
	if (strcmp(ptr, "1") == 0) {
		ptr = strtok(NULL, "/");
		strcpy(id, ptr); //클라이언트가 입력한 삭제할 ID를 복사

		WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
		for (int i = 0; i < userCount; i++) {
			if (strcmp(users[i].id, id) == 0) { //삭제할 ID를 찾으면
				for (int j = i; j < userCount - 1; j++) { //해당 ID 뒤에 있는 사용자 명 수만큼
					//한 칸 씩 앞으로 당김
					strcpy(users[i].id, users[i + 1].id);
					strcpy(users[i].pw, users[i + 1].pw);
				}

				userCount--;
				ReleaseMutex(hMutex); //뮤텍스 중지
				break;
			}
		}

		UserFileUpdate(); //사용자 정보 파일 업데이트
	}
	else {
		return;
	}
}

//사용자 정보 추가 함수
void UserAdd(SOCKET clientSock, char* ptr) {
	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	ptr = strtok(NULL, "/");
	strcpy(users[userCount].id, ptr); //사용자가 입력한 추가할 ID를 복사

	ptr = strtok(NULL, "/");
	strcpy(users[userCount].pw, ptr); //사용자가 입력한 추가할 PW를 복사

	userCount++; //사용자 명 수를 1 증가
	ReleaseMutex(hMutex); //뮤텍스 중지
	UserFileUpdate(); //사용자 정보 파일 업데이트
}

//도서 정보 파일 업데이트 함수
void BookFileUpdate() {
	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	FILE* bookfp;
	bookfp = fopen("booklist2.txt", "w"); //도서 정보 파일을 쓰기 모드로 엶
	if (bookfp == NULL) {
		ErrorHandling("파일 탐색에 실패하였습니다!");
	}

	//반복문으로 프로그램이 가지고 있는 구조체의 정보를 파일에 덮어씀
	for (int i = 0; i < bookCount; i++) {
		fprintf(bookfp, "%d\t%s\t%s\t%.2f\n", books[i].num, books[i].bookTitle, books[i].author, books[i].rating);
	}

	fclose(bookfp); //파일을 닫음
	ReleaseMutex(hMutex); //뮤텍스 중지
}

//사용자 정보 파일 업데이트 함수
void UserFileUpdate() {
	WaitForSingleObject(hMutex, INFINITE); //뮤텍스 실행
	FILE* userfp;
	userfp = fopen("users.txt", "w"); //사용자 정보 파일을 쓰기 모드로 엶
	if (userfp == NULL) {
		ErrorHandling("파일 탐색에 실패하였습니다!");
	}

	//반복문으로 프로그램이 가지고 있는 구조체의 정보를 파일에 덮어씀
	for (int i = 0; i < userCount; i++) {
		fprintf(userfp, "%s//%s\n", users[i].id, users[i].pw);
	}

	fclose(userfp); //파일을 닫음
	ReleaseMutex(hMutex); //뮤텍스 중지
}

//오류 상황 처리 함수
void ErrorHandling(char* msg) {
	fputs(msg, stderr); //매개변수로 받은 메시지를 출력
	fputc('\n', stderr); //개행 문자 출력
	exit(1); //프로그램 종료
}
