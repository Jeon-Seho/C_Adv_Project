#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <process.h>

#define MAX_BUF 8192 //�ִ� ���� ũ��
#define MAX_CLNT 256 //�ִ� ���� ���� Ŭ���̾�Ʈ
#define MAX_BOOK 700 //�ִ� ���� �Ǽ�
#define MAX_USER 20 //�ִ� ����� ��
#define MAX_STR 100 //�ִ� ���ڿ� ����
#define MAX_LEN 20 //�ִ� ID, PW ����

unsigned WINAPI HandleClient(void* arg); //Ŭ���̾�Ʈ�� �����ϴ� ������ �Լ�

int LoginProcess(SOCKET clientSock, char* recvMsg); //�α��� ó�� �Լ�
void BookSearch(SOCKET clientSock, char* ptr); //���� ���� �˻� �Լ�
void BookEdit(SOCKET clientSock, int findIndex, char* ptr); //���� ���� ���� �Լ�
void BookDelete(SOCKET clientSock, int findIndex, char* ptr); //���� ���� ���� �Լ�
void BookAdd(SOCKET clientSock, char* ptr); //���� ���� �߰� �Լ�
void BookRanking(SOCKET clientSock, char* ptr); //���� �� ��ŷ ���� �Լ�
void UserEdit(SOCKET clientSock, char* ptr); //����� ���� ���� �Լ�
void UserDelete(SOCKET clientSock, char* ptr); //����� ���� ���� �Լ�
void UserAdd(SOCKET clientSock, char* ptr); //����� ���� �߰� �Լ�

void BookFileUpdate(); //���� ���� ���� ������Ʈ �Լ�
void UserFileUpdate(); //����� ���� ���� ������Ʈ �Լ�

void ErrorHandling(char* msg); //���� ��Ȳ ó�� �Լ�

//���� ������ �����ϴ� ����ü
typedef struct bookInfo {
	int num; //����
	char bookTitle[MAX_STR]; //������
	char author[MAX_STR]; //���ڸ�
	float rating; //����
} BOOK;

//����� ������ �����ϴ� ����ü
typedef struct userInfo {
	char id[MAX_LEN]; //ID
	char pw[MAX_LEN]; //PW
} USER;

//���� ����
SOCKET clientSocks[MAX_CLNT]; //Ŭ���̾�Ʈ ���� ���� �迭
BOOK books[MAX_BOOK]; //���� ���� ���� �迭
BOOK* sortedBooks[MAX_BOOK]; //���ĵ� ���� ������ �ּҸ� �����ϴ� �迭
USER users[MAX_USER]; //����� ���� ���� �迭
HANDLE hMutex; //���ؽ�

int clientCount = 0; //Ŭ���̾�Ʈ ����
int bookCount = 0; //���� �� ��
int userCount = 0; //����� �� ��

int msgLen = 0; //recv�� �޽��� ������ ũ�⸦ ����
char recvMsg[MAX_BUF], sendMsg[MAX_BUF];

int main() {
	WSADATA wsaData;
	SOCKET serverSock, clientSock;
	SOCKADDR_IN serverAddr, clientAddr;
	int clientAddrSize;
	HANDLE hThread;

	char port[100] = "55555";
	char bookData[MAX_STR * 2], userData[MAX_LEN * 2];

	//���� ���� ���� �б�
	FILE* bookfp;
	bookfp = fopen("booklist2.txt", "r"); //���� ���� ������ �б� ���� ��
	if (bookfp == NULL) { //������ ã�� ���Ͽ� NULL�� ��ȯ�� ���
		ErrorHandling("���� Ž���� �����Ͽ����ϴ�!");
	}

	//���� ���� ����
	while (fgets(bookData, sizeof(bookData), bookfp) != NULL) {
		//���๮�� ����
		for (int i = 0; i < sizeof(bookData); i++) {
			if (bookData[i] == '\n') {
				bookData[i] = '\0';
				break;
			}
		}

		char* ptr = strtok(bookData, "\t");
		books[bookCount].num = atoi(ptr); //���� ����
		ptr = strtok(NULL, "\t");
		strcpy(books[bookCount].bookTitle, ptr); //������ ����
		ptr = strtok(NULL, "\t");
		strcpy(books[bookCount].author, ptr); //���ڸ� ����
		ptr = strtok(NULL, "\t");
		books[bookCount].rating = atof(ptr); //���� ����

		bookCount++; //���� �� ���� 1 ����
	}
	fclose(bookfp); //������ ����

	//����� ���� ���� �б�
	FILE* userfp;
	userfp = fopen("users.txt", "r"); //����� ���� ������ �б� ���� ��
	if (userfp == NULL) { //������ ã�� ���Ͽ� NULL�� ��ȯ�� ���
		ErrorHandling("���� Ž���� �����Ͽ����ϴ�!");
	}

	//����� ���� ����
	while (fgets(userData, sizeof(userData), userfp) != NULL) {
		//���๮�� ����
		for (int i = 0; i < sizeof(userData); i++) {
			if (userData[i] == '\n') {
				userData[i] = '\0';
				break;
			}
		}

		char* ptr = strtok(userData, "//");
		strcpy(users[userCount].id, ptr); //ID ����
		ptr = strtok(NULL, "//");
		strcpy(users[userCount].pw, ptr); //PW ����

		userCount++; //����� �� ���� 1 ����
	}
	fclose(userfp); //������ ����

	//��Ʈ �Է�
	/*printf("Input port number : ");
	gets(port);*/

	//���� ��� ����
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	// ���ؽ� ����
	hMutex = CreateMutex(NULL, FALSE, NULL);

	//���� ����
	serverSock = socket(PF_INET, SOCK_STREAM, 0);

	//���� �ּ� ���� ����
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(atoi(port));

	//���� ��ġ �� �غ�
	if (bind(serverSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	if (listen(serverSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");

	printf("listening...\n");

	//Ŭ���̾�Ʈ ������ ����
	while (1) {
		clientAddrSize = sizeof(clientAddr);
		clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &clientAddrSize); //�������� ���޵� Ŭ���̾�Ʈ ������ clientSock�� ����
		WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
		clientSocks[clientCount++] = clientSock; //Ŭ���̾�Ʈ ���Ϲ迭�� ��� ������ ���� �ּҸ� ����, Ŭ���̾�Ʈ ���� 1 ����
		ReleaseMutex(hMutex); //���ؽ� ����
		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (void*)&clientSock, 0, NULL); //HandleClient ������ ����, clientSock�� �Ű������� ����
		printf("Connected Client IP : %s\n", inet_ntoa(clientAddr.sin_addr)); //������ Ŭ���̾�Ʈ�� IP ���
	}

	//���� ���� ó��
	closesocket(serverSock);
	WSACleanup();

	//���α׷� ����
	return 0;
}

unsigned WINAPI HandleClient(void* arg) {
	SOCKET clientSock = *((SOCKET*)arg); //�Ű������� ���� Ŭ���̾�Ʈ ������ ����
	
	//�α��� ó�� �ݺ���
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) {
		recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

		//�α��ο� �������� ���
		if (LoginProcess(clientSock, recvMsg)) {
			sprintf(sendMsg, "LOGIN/1/SUCCESS");
			send(clientSock, sendMsg, strlen(sendMsg), 0); //�α��� ���� ��ȣ send
			break; //�α��� �ݺ������� break
		}
		//�α��ο� �������� ���
		else {
			sprintf(sendMsg, "LOGIN/1/FAIL");
			send(clientSock, sendMsg, strlen(sendMsg), 0); //�α��� ���� ��ȣ send
		}
	}

	//��� ���� �ݺ���
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) {
		recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����
		char* ptr = strtok(recvMsg, "/");

		//���� ���� ���� ���
		if (strcmp(ptr, "Book") == 0) {
			ptr = strtok(NULL, "/");

			//���� ���� �˻� ���
			if (strcmp(ptr, "Search") == 0) {
				BookSearch(clientSock, ptr);
			}
			//���� ���� �߰� ���
			else if (strcmp(ptr, "Add") == 0) {
				BookAdd(clientSock, ptr);
			}
			//���� ���� �� ��ŷ ���� ���
			else if (strcmp(ptr, "Ranking") == 0) {
				BookRanking(clientSock, ptr);
			}
		}
		//����� ���� ���� ���
		else if (strcmp(ptr, "User") == 0) {
			ptr = strtok(NULL, "/");

			//����� ���� ���� ���
			if (strcmp(ptr, "Edit") == 0) {
				UserEdit(clientSock, ptr);

			}
			//����� ���� ���� ���
			else if (strcmp(ptr, "Delete") == 0) {
				UserDelete(clientSock, ptr);
			}
			//����� ���� �߰� ���
			else if (strcmp(ptr, "Add") == 0) {
				UserAdd(clientSock, ptr);
			}
		}
		//Ŭ���̾�Ʈ ���� ���
		else if (strcmp(ptr, "EXIT") == 0) {
			//Ŭ���̾�Ʈ ���� ����
			printf("client left the program\n");
			WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
			for (int i = 0; i < clientCount; i++) { //Ŭ���̾�Ʈ�� ������ŭ �ݺ�
				if (clientSock == clientSocks[i]) { //���� ���� clientSock���� �迭�� ���� ���ٸ�
					while (i++ < clientCount - 1) //Ŭ���̾�Ʈ ���� ��ŭ
						clientSocks[i] = clientSocks[i + 1]; //������ ���
					break;
				}
			}
			clientCount--; //Ŭ���̾�Ʈ ���� �ϳ� ����
			ReleaseMutex(hMutex); //���ؽ� ����
			closesocket(clientSock); //Ŭ���̾�Ʈ ���� ����
			break;
		}
		else {
			//����ó��
		}
	}
	return 0;
}

//�α��� ó�� �Լ�
int LoginProcess(SOCKET clientSock, char* recvMsg) {
	char id[MAX_STR], pw[MAX_STR];
	int loginCheck = 0; //�α��� ���� ���θ� Ȯ���ϴ� ����

	//Ŭ���̾�Ʈ�� ���� ��ȣ�� �α��� �õ��� �ƴ� ���
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
	strcpy(id, ptr); // Ŭ���̾�Ʈ�� �Է��� ID ����
	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); // Ŭ���̾�Ʈ�� �Է��� PW ����

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	//Ŭ���̾�Ʈ�� �Է��� ID�� PW�� ����� ������ ��� �ִ� ����ü ����� ��
	for (int i = 0; i < userCount; i++) {
		//ID�� PW�� ��� ��ġ�� ��
		if (strcmp(users[i].id, id) == 0 && strcmp(users[i].pw, pw) == 0) {
			loginCheck = 1; //�α��� ���� üũ
			break;
		}
	}
	ReleaseMutex(hMutex); //���ؽ� ����

	if (loginCheck) {
		return 1;
	}
	else {
		return 0;
	}
}

//���� ���� �˻� �Լ�
void BookSearch(SOCKET clientSock, char* ptr) {
	int findIndex = -1; //�˻��� ���� ������ �ε���
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	//���� �� �� ��ŭ �ݺ�
	for (int i = 0; i < bookCount; i++) {
		//Ŭ���̾�Ʈ�� �Է��� ������� ����ü�� �������� ������
		if (strcmp(ptr, books[i].bookTitle) == 0) {
			findIndex = i; // �˻��� ���� ���� �ε����� ã�� �ε��� i�� ����
			break;
		}
	}
	ReleaseMutex(hMutex); //���ؽ� ����

	//������ �������� ã�� ���ߴٸ�
	if (findIndex == -1) {
		sprintf(sendMsg, "Book/Search/0");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //�˻� ���� ��ȣ send
	}
	//������ �������� ã�Ҵٸ�
	else {
		sprintf(sendMsg, "Book/Search/1/\n����: [%d], ����: [%s], ����: [%s], ����: [%.2f]\n", books[findIndex].num, books[findIndex].bookTitle, books[findIndex].author, books[findIndex].rating);
		send(clientSock, sendMsg, strlen(sendMsg), 0); //���� ������ send

		msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //Ŭ���̾�Ʈ�κ��� ���� ����, ���� ���θ� recv
		recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����
		char* ptr = strtok(recvMsg, "/");

		if (strcmp(ptr, "Book") == 0) {
			ptr = strtok(NULL, "/");

			if (strcmp(ptr, "Edit") == 0) {
				BookEdit(clientSock, findIndex, ptr); //���� ���� ���� �Լ� ȣ��
			}
			else if (strcmp(ptr, "Delete") == 0) {
				BookDelete(clientSock, findIndex, ptr); //���� ���� ���� �Լ� ȣ��
			}
		}
	}
}

//���� ���� ���� �Լ�
void BookEdit(SOCKET clientSock, int findIndex, char* ptr) {
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����

	//�������� �����ϴ� ���
	if (strcmp(ptr, "1") == 0) {
		ptr = strtok(NULL, "/");

		strcpy(books[findIndex].bookTitle, ptr); //������ ����
	}
	//���ڸ��� �����ϴ� ���
	else if (strcmp(ptr, "2") == 0) {
		ptr = strtok(NULL, "/");

		strcpy(books[findIndex].author, ptr); //���ڸ� ����
	}
	//������ �����ϴ� ���
	else if (strcmp(ptr, "3") == 0) {
		ptr = strtok(NULL, "/");

		books[findIndex].rating = atof(ptr); //���� ����
	}

	ReleaseMutex(hMutex); //���ؽ� ����
	BookFileUpdate(); //���� ���� ���� ������Ʈ
}

//���� ���� ���� �Լ�
void BookDelete(SOCKET clientSock, int findIndex, char* ptr) {
	ptr = strtok(NULL, "/");

	//���� ������ ���������� �����ߴٸ�
	if (strcmp(ptr, "1") == 0) {
		WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
		//������ ������ �ε������� ������ ��������
		for (int i = findIndex; i < bookCount - 1; i++) {
			//������ �� ĭ �� ���
			strcpy(books[i].bookTitle, books[i + 1].bookTitle);
			strcpy(books[i].author, books[i + 1].author);
			books[i].rating = books[i + 1].rating;
		}

		bookCount--; //���� �� ���� 1 ����
		ReleaseMutex(hMutex); //���ؽ� ����
		BookFileUpdate(); //���� ���� ���� ������Ʈ

	}
	//���� ������ ���߿� ����ߴٸ�
	else if (strcmp(ptr, "0") == 0) {
		return;
	}
}

//���� ���� �߰� �Լ�
void BookAdd(SOCKET clientSock, char* ptr) {
	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	//���� ����ü�� ���� �������� ���ο� ������ �߰�
	books[bookCount].num = bookCount + 1;

	ptr = strtok(NULL, "/");
	strcpy(books[bookCount].bookTitle, ptr); //Ŭ���̾�Ʈ�� �Է��� ������ ����

	ptr = strtok(NULL, "/");
	strcpy(books[bookCount].author, ptr); //Ŭ���̾�Ʈ�� �Է��� ���ڸ� ����

	ptr = strtok(NULL, "/");
	books[bookCount].rating = atof(ptr); //Ŭ���̾�Ʈ�� �Է��� ���� ����

	bookCount++; //���� �� ���� 1 ����
	ReleaseMutex(hMutex); //���ؽ� ����

	BookFileUpdate(); //���� ���� ���� ������Ʈ
}

//���� �� ��ŷ ���� �Լ�
void BookRanking(SOCKET clientSock, char* ptr) {
	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	for (int i = 0; i < bookCount; i++) {
		sortedBooks[i] = &books[i]; //����ü ������ �迭�� ��� ����ü�� �ּҸ� ����
	}

	//���� ����(��������)
	for (int i = 0; i < bookCount - 1; i++) {
		for (int j = 0; j < bookCount - i - 1; j++) {
			//���� �ּ��� �������� ���� �ּ��� ������ �� ũ�ٸ� 
			if (sortedBooks[j]->rating < sortedBooks[j + 1]->rating) {

				//swap
				BOOK* tempPtr = sortedBooks[j];
				sortedBooks[j] = sortedBooks[j + 1];
				sortedBooks[j + 1] = tempPtr;
			}
		}
	}

	sendMsg[0] = '\0'; //sendMsg �ʱ�ȭ
	for (int i = 0; i < 30; i++) {
		char tempMsg[1024];

		sprintf(tempMsg, "����: [%d], ����: [%s], ����: [%s], ����: [%.2f]\n", sortedBooks[i]->num, sortedBooks[i]->bookTitle, sortedBooks[i]->author, sortedBooks[i]->rating);
		strcat(sendMsg, tempMsg); //���� ������ ������ �����͸� �̾����
	}
	send(clientSock, sendMsg, strlen(sendMsg), 0); //�̾���� �����͸� �۽�
	ReleaseMutex(hMutex); //���ؽ� ����
}

//����� ���� ���� �Լ�
void UserEdit(SOCKET clientSock, char* ptr) {
	char id[MAX_STR], pw[MAX_STR];
	char newPw[MAX_STR];
	int check = 0;

	ptr = strtok(NULL, "/");
	strcpy(id, ptr); //Ŭ���̾�Ʈ�� �Է��� ID ����

	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); //Ŭ���̾�Ʈ�� �Է��� PW ����

	ptr = strtok(NULL, "/");
	strcpy(newPw, ptr); //Ŭ���̾�Ʈ�� �Է��� ���ο� PW ����

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	for (int i = 0; i < userCount; i++) {
		//id�� pw�� ��ġ�ϴٸ�
		if (strcmp(users[i].id, id) == 0 && strcmp(users[i].pw, pw) == 0) {
			strcpy(users[i].pw, newPw); //pw�� ���ο� pw�� ����
			check = 1; //����� ���� ���� ���� üũ
		}
	}
	ReleaseMutex(hMutex); //���ؽ� ����
	UserFileUpdate(); //����� ���� ���� ������Ʈ

	//����� ���� ���濡 �������� ��
	if (check) {
		sprintf(sendMsg, "User/Edit/1");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //����� ���� ���� ���� ��ȣ �۽�
	}
	else {
		sprintf(sendMsg, "User/Edit/0");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //����� ���� ���� ���� ��ȣ �۽�
	}
}

//����� ���� ���� �Լ�
void UserDelete(SOCKET clientSock, char* ptr) {
	char id[MAX_STR], pw[MAX_STR];
	ptr = strtok(NULL, "/");

	//����� ���� ������ ���������� �����ߴٸ�
	if (strcmp(ptr, "1") == 0) {
		ptr = strtok(NULL, "/");
		strcpy(id, ptr); //Ŭ���̾�Ʈ�� �Է��� ������ ID�� ����

		WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
		for (int i = 0; i < userCount; i++) {
			if (strcmp(users[i].id, id) == 0) { //������ ID�� ã����
				for (int j = i; j < userCount - 1; j++) { //�ش� ID �ڿ� �ִ� ����� �� ����ŭ
					//�� ĭ �� ������ ���
					strcpy(users[i].id, users[i + 1].id);
					strcpy(users[i].pw, users[i + 1].pw);
				}

				userCount--;
				ReleaseMutex(hMutex); //���ؽ� ����
				break;
			}
		}

		UserFileUpdate(); //����� ���� ���� ������Ʈ
	}
	else {
		return;
	}
}

//����� ���� �߰� �Լ�
void UserAdd(SOCKET clientSock, char* ptr) {
	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	ptr = strtok(NULL, "/");
	strcpy(users[userCount].id, ptr); //����ڰ� �Է��� �߰��� ID�� ����

	ptr = strtok(NULL, "/");
	strcpy(users[userCount].pw, ptr); //����ڰ� �Է��� �߰��� PW�� ����

	userCount++; //����� �� ���� 1 ����
	ReleaseMutex(hMutex); //���ؽ� ����
	UserFileUpdate(); //����� ���� ���� ������Ʈ
}

//���� ���� ���� ������Ʈ �Լ�
void BookFileUpdate() {
	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	FILE* bookfp;
	bookfp = fopen("booklist2.txt", "w"); //���� ���� ������ ���� ���� ��
	if (bookfp == NULL) {
		ErrorHandling("���� Ž���� �����Ͽ����ϴ�!");
	}

	//�ݺ������� ���α׷��� ������ �ִ� ����ü�� ������ ���Ͽ� ���
	for (int i = 0; i < bookCount; i++) {
		fprintf(bookfp, "%d\t%s\t%s\t%.2f\n", books[i].num, books[i].bookTitle, books[i].author, books[i].rating);
	}

	fclose(bookfp); //������ ����
	ReleaseMutex(hMutex); //���ؽ� ����
}

//����� ���� ���� ������Ʈ �Լ�
void UserFileUpdate() {
	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	FILE* userfp;
	userfp = fopen("users.txt", "w"); //����� ���� ������ ���� ���� ��
	if (userfp == NULL) {
		ErrorHandling("���� Ž���� �����Ͽ����ϴ�!");
	}

	//�ݺ������� ���α׷��� ������ �ִ� ����ü�� ������ ���Ͽ� ���
	for (int i = 0; i < userCount; i++) {
		fprintf(userfp, "%s//%s\n", users[i].id, users[i].pw);
	}

	fclose(userfp); //������ ����
	ReleaseMutex(hMutex); //���ؽ� ����
}

//���� ��Ȳ ó�� �Լ�
void ErrorHandling(char* msg) {
	fputs(msg, stderr); //�Ű������� ���� �޽����� ���
	fputc('\n', stderr); //���� ���� ���
	exit(1); //���α׷� ����
}