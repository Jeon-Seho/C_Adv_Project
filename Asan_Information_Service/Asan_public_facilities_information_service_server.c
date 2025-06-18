#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include <time.h>

#define MAX_BUF 8192 //�ִ� ���� ũ��
#define MAX_CLNT 256 //�ִ� ���� ���� Ŭ���̾�Ʈ
#define MAX_STR 100 //�ִ� ���ڿ� ����
#define MAX_LEN 20 //�ִ� ID, PW ����
#define FILE_CNT 6 //���� ����
#define PAGE 5 //������ ����

//���Ḯ��Ʈ ����ü
typedef struct INFO {
	int num;				//����
	char name[MAX_STR];		//�ü���
	char address[MAX_STR];	//�ü� �ּ�
	char note[MAX_LEN];		//���
	struct INFO* prev;		//�ڱ� ���� ����ü
	struct INFO* next;		//�ڱ� ���� ����ü
} INFO;

typedef struct USER {
	char id[MAX_LEN];	//ID
	char pw[MAX_LEN];	//PW
	int permission;		//����
	struct USER* next;	//�ڱ� ���� ����ü
} USER;

typedef struct REVIEW {
	int division;				//����
	int num;					//����
	char text[MAX_STR * 10];	//�ü� �ı� ����
	struct REVIEW* next;		//�ڱ� ���� ����ü
} REVIEW;

unsigned WINAPI HandleClient(void* arg); //Ŭ���̾�Ʈ ���� ������ �Լ�
void ErrorHandling(char* msg); //���� ��Ȳ ó�� �Լ�

void InfoFileRead(); //�����ü� ���� ���� ���� �Լ�
void UserFileRead(); //����� ���� ���� ���� �Լ�
void ReviewFileRead(); //�ü� �ı� ���� ���� ���� �Լ�
void InfoFileUpdate(int num); //�����ü� ���� ���� ������Ʈ �Լ�
void UserFileUpdate(); //����� ���� ���� ������Ʈ �Լ�
void ReviewFileUpdate(); //�ü� �ı� ���� ���� ������Ʈ �Լ�
void InfoFree(); //�����ü� ���� ����Ʈ �޸� ���� �Լ�
void UserFree(); //����� ���� ����Ʈ �޸� ���� �Լ�
void ReviewFree(); //�ü� �ı� ���� ����Ʈ �޸� ���� �Լ�

void SignUpProcess(SOCKET clientSock, char* ptr); //ȸ������ �Լ�
int LoginProcess(SOCKET clientSock, char* ptr); //�α��� �Լ�

void ReviewWrite(SOCKET clientSock, INFO* select_facility, int division, char* ptr); //�ü� �ı� �ۼ� �Լ�
void InfoEdit(SOCKET clientSock, INFO* select_facility, int division, char* ptr); //�ü� ���� ���� �Լ�
void UserEdit(SOCKET clientSock, USER* current_user, char* ptr); //����� ���� ���� �Լ�
void UserDelete(SOCKET clientSock, char* ptr); //����� ���� ���� �Լ�

//����Ʈ ��� �ּ�
INFO* disabled_head = NULL;
INFO* local_head = NULL;
INFO* old_head = NULL;
INFO* park_head = NULL;
INFO* rest_head = NULL;
INFO* hall_head = NULL;
USER* user_head = NULL;
REVIEW* review_head = NULL;

//��� �ּҸ� �����ϴ� ���� ������ �迭
INFO** all_heads[FILE_CNT] = {&disabled_head, &local_head, &old_head, &park_head, &rest_head, &hall_head};

//���ϸ��� �����ϴ� ���ڿ� ������ �迭
char* all_files[FILE_CNT] = {
		"Asan_disabled_facility.csv",
		"Asan_local_government.csv",
		"Asan_old_facility.csv",
		"Asan_parking_lot.csv",
		"Asan_rest_area.csv",
		"Asan_town_hall.csv",
};

//���� ����
SOCKET serverSock;
SOCKET clientSocks[MAX_CLNT]; //Ŭ���̾�Ʈ ���� ���� �迭
HANDLE hMutex; //���ؽ�
int clientCount = 0; //Ŭ���̾�Ʈ ���� ����
int serverExit = 0;

int main() {
	WSADATA wsaData;
	SOCKET clientSock;
	SOCKADDR_IN serverAddr, clientAddr;
	int clientAddrSize;
	HANDLE hThread, hInputThread;

	char port[100] = "55555";

	//���� �б� �� ����
	InfoFileRead(); //�����ü� ���� ���� �Լ� ȣ��
	UserFileRead(); //����� ���� ���� �Լ� ȣ��
	ReviewFileRead(); //�ü� �ı� ���� ���� �Լ� ȣ��

	//���� ��� ����
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup() error!");
	}
	
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
	if (bind(serverSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		ErrorHandling("bind() error");
	}
	if (listen(serverSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error");
	}
		
	printf("server start!\n");

	//Ŭ���̾�Ʈ ������ ����
	while (serverExit == 0) {
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

	//����Ʈ �޸� ����
	InfoFree();
	UserFree();
	ReviewFree();

	//���α׷� ����
	return 0;
}

//Ŭ���̾�Ʈ ���� ������ �Լ�
unsigned WINAPI HandleClient(void* arg) {
	SOCKET clientSock = *((SOCKET*)arg); //�Ű������� ���� Ŭ���̾�Ʈ ������ ����
	char recvMsg[MAX_BUF], sendMsg[MAX_BUF]; //���� ���� �� �۽� ���� ����
	int msgLen = 0; //recv�� �޽��� ������ ũ�⸦ �����ϴ� ����

	//�α���, ��ȸ��, ȸ������ �ݺ���
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) { //Ŭ���̾�Ʈ�κ��� �޽��� ����
		recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

		char* ptr = strtok(recvMsg, "/");

		if (strcmp(ptr, "Login") == 0) {
			ptr = strtok(NULL, "/");

			//ȸ������
			if (strcmp(ptr, "new") == 0) {
				SignUpProcess(clientSock, ptr);
			}
			//�α���
			else if (strcmp(ptr, "member") == 0) {
				if (LoginProcess(clientSock, ptr)) {
					break;
				}
			}
			//��ȸ��
			else if (strcmp(ptr, "guest") == 0) {
				sprintf(sendMsg, "Login/guest/success");
				send(clientSock, sendMsg, strlen(sendMsg), 0);
				break;
			}
		}
		//���� ó��
		else {
			
		}
	}

	//��� ���� �ݺ���
	while ((msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0)) != 0) { //Ŭ���̾�Ʈ�κ��� �޽��� ����
		recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

		char* ptr = strtok(recvMsg, "/");

		//�����ü� ���� ��ȸ
		if (strcmp(ptr, "Info") == 0) {
			int division;
			char facilityName[MAX_STR];
			char facilityAddress[MAX_STR];
			
			ptr = strtok(NULL, "/");

			if (strcmp(ptr, "division") == 0) {
				ptr = strtok(NULL, "/");
				division = atoi(ptr) - 1;
			}

			WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
			INFO* current_info = *(all_heads[division]);
			ReleaseMutex(hMutex); //���ؽ� ����

			int startCheck = 0, endCheck = 0;
			while (1) {
				WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
				//5�� ������ �ü��� ��� ���
				sendMsg[0] = '\0'; //sendMsg �ʱ�ȭ
				for (int i = 0; i < PAGE; i++) {
					char tmpMsg[1024];

					sprintf(tmpMsg, "%d. %s ", current_info->num, current_info->name);
					strcat(sendMsg, tmpMsg); //�ü� ��� ����

					//������ ������� Ȯ��
					if (current_info->next == NULL) {
						endCheck = 1;
						break;
					}
					else {
						current_info = current_info->next;
					}
				}
				ReleaseMutex(hMutex); //���ؽ� ����
				
				if (startCheck) {
					strcat(sendMsg, "/start"); //ó�� �������� �ǹ��ϴ� �÷��� �߰�
					startCheck = 0;
				}
				else if (endCheck) {
					strcat(sendMsg, "/end"); //������ �������� �ǹ��ϴ� �÷��� �߰�
					endCheck = 0;
				}
				else {
					strcat(sendMsg, "/normal");
				}
				send(clientSock, sendMsg, strlen(sendMsg), 0);

				//����, ����, ����, �˻�
				msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //Ŭ���̾�Ʈ�κ��� �޽��� ����
				recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

				ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "Info") == 0) {
					ptr = strtok(NULL, "/");

					//���� ������
					if (strcmp(ptr, "prev") == 0) {
						WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
						//�ּҸ� 10ĭ �������� �̵�
						for (int i = 0; i < PAGE * 2; i++) {
							//ù��° ������� Ȯ��
							if (current_info == *(all_heads[division])) {
								startCheck = 1;
								break;
							}
							else {
								current_info = current_info->prev;
							}
						}
						ReleaseMutex(hMutex); //���ؽ� ����
					}
					//���� ������
					else if (strcmp(ptr, "next") == 0) {
						continue;
					}
					//�ּ� �˻�
					else if (strcmp(ptr, "search") == 0) {
						ptr = strtok(NULL, "/");
						strcpy(facilityAddress, ptr);

						//strstr �Լ��� ����Ͽ� �ش� division�� ������ �ּҸ� ���� ��尡 �ִ��� Ȯ��
						sendMsg[0] = '\0'; //sendMsg �ʱ�ȭ
						current_info = *(all_heads[division]);
						while (current_info != NULL) {
							if (strstr(current_info->address, facilityAddress) != NULL) {
								char tmpMsg[1024];

								sprintf(tmpMsg, "[%d. %s]\n", current_info->num, current_info->name);
								strcat(sendMsg, tmpMsg);
							}

							current_info = current_info->next;
						}

						//���� ��尡 �ϳ��� ���ٸ� �ȳ� �۽�
						if (sendMsg[0] == '\0') {
							sprintf(sendMsg, "Info/search/fail");
							send(clientSock, sendMsg, strlen(sendMsg), 0);

							continue;
						}
						//�˻� ����� �۽�
						else {
							send(clientSock, sendMsg, strlen(sendMsg), 0);

							//�ü����� select ����
							msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //Ŭ���̾�Ʈ�κ��� �޽��� ����
							recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

							ptr = strtok(recvMsg, "/");
							if (strcmp(ptr, "Info") == 0) {
								ptr = strtok(NULL, "/");

								if (strcmp(ptr, "select") == 0) {
									ptr = strtok(NULL, "/");
									strcpy(facilityName, ptr); //�Է��� �ü����� ����

									break;
								}
							}
						}
					}
					//�ü��� ����
					else if (strcmp(ptr, "select") == 0) {
						ptr = strtok(NULL, "/");
						
						strcpy(facilityName, ptr); //�Է��� �ü����� ����
						break;
					}
					//���� ó��
					else {

					}
				}
			}

			WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
			//������ �ü��� Ž��
			INFO* select_facility = *(all_heads[division]);
			while (select_facility != NULL) {
				if (strcmp(select_facility->name, facilityName) == 0) {
					break;
				}
				select_facility = select_facility->next;
			}
			ReleaseMutex(hMutex); //���ؽ� ����

			//�Է��� �ü����� ����Ʈ�� �������� ������
			if (select_facility == NULL) {
				sprintf(sendMsg, "Info/select/fail");
				send(clientSock, sendMsg, strlen(sendMsg), 0);
				continue;
			}

			//������ �ü��� �� ���� �۽�
			sprintf(sendMsg, "Info/select/�ü���: %s | �ּ�: %s | ���: %s", select_facility->name, select_facility->address, select_facility->note);
			send(clientSock, sendMsg, strlen(sendMsg), 0);

			WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
			//�ı� ���� Ž�� �� �۽�
			sendMsg[0] = '\0'; //sendMsg �ʱ�ȭ
			REVIEW* current_review = review_head;
			while (current_review != NULL) {
				if (current_review->division == division && current_review->num == select_facility->num) {
					char tmpMsg[1024];

					sprintf(tmpMsg, "%s\n", current_review->text);
					strcat(sendMsg, tmpMsg);
				}
				current_review = current_review->next;
			}
			ReleaseMutex(hMutex); //���ؽ� ����

			//���䵥���Ͱ� ���� ���
			if (sendMsg[0] == '\0') {
				sprintf(sendMsg, "�ı� ������ �����ϴ�!\n");
			}
			send(clientSock, sendMsg, strlen(sendMsg), 0); //������ �۽�

			//��� ����(1. �ı� �ۼ�, 2. �ü� ���� ����, 3. ����)
			msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //Ŭ���̾�Ʈ�κ��� �޽��� ����
			recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

			ptr = strtok(recvMsg, "/");
			
			if (strcmp(ptr, "Info") == 0) {
				ptr = strtok(NULL, "/");

				//�ü� �ı� �ۼ�
				if (strcmp(ptr, "review") == 0) {
					ReviewWrite(clientSock, select_facility, division, ptr);
				}
				//�ü� ���� ����
				else if (strcmp(ptr, "edit") == 0) {
					InfoEdit(clientSock, select_facility, division, ptr);
				}
				//����
				else if (strcmp(ptr, "end") == 0) {
					continue;
				}
				//���� ó��
				else {

				}
			}
		}
		//����� ���� ����
		else if (strcmp(ptr, "User") == 0) {
			char id[MAX_LEN], pw[MAX_LEN];
			
			ptr = strtok(NULL, "/");

			//��й�ȣ ��ġ ���� Ȯ��
			if (strcmp(ptr, "checkpw") == 0) {
				ptr = strtok(NULL, "/");
				strcpy(id, ptr);

				ptr = strtok(NULL, "/");
				strcpy(pw, ptr);
			}

			//����Ʈ���� �ش� ID�� Ž��
			WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
			USER* tmp = user_head;
			while (tmp != NULL) {
				if (strcmp(tmp->id, id) == 0) {
					break;
				}
				tmp = tmp->next;
			}

			//�ش� ID�� PW�� Ŭ���̾�Ʈ�� �Է��� PW�� �������� Ȯ��
			if (strcmp(tmp->pw, pw) == 0) { //�����ϴٸ� ���� ��ȣ �۽�
				
				ReleaseMutex(hMutex); //���ؽ� ����
				sprintf(sendMsg, "User/checkpw/success");
				send(clientSock, sendMsg, strlen(sendMsg), 0);

				msgLen = recv(clientSock, recvMsg, sizeof(recvMsg), 0); //Ŭ���̾�Ʈ�κ��� �޽��� ����
				recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

				ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "User") == 0) {
					ptr = strtok(NULL, "/");
					
					//����� ���� ����
					if (strcmp(ptr, "edit") == 0) {
						UserEdit(clientSock, tmp, ptr);
					}
					//����� ���� ����
					else if (strcmp(ptr, "delete") == 0) {
						UserDelete(clientSock, ptr);
					}
					//���� ó��
					else {

					}
				}
				//���� ó��
				else {

				}
			}
			//�������� �ʴٸ� ���� ��ȣ �۽�
			else {
				ReleaseMutex(hMutex); //���ؽ� ����
				sprintf(sendMsg, "User/checkpw/fail");
				send(clientSock, sendMsg, strlen(sendMsg), 0);
			}
		}
		//���α׷� ����
		else if (strcmp(ptr, "Exit") == 0) {
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
		//���� ó��
		else {
			
		}
	}

	return 0;
}

//�����ü� ���� ���� ���� �Լ�
void InfoFileRead() {
	FILE* fp;
	char tmpData[MAX_STR * 5]; //�ӽ� ���ڿ� ���� ����

	for (int i = 0; i < FILE_CNT; i++) {
		fp = fopen(all_files[i], "r");
		if (fp == NULL) {
			ErrorHandling("�����ü� ���� ���� Ž���� �����Ͽ����ϴ�!");
		}

		fgets(tmpData, sizeof(tmpData), fp); //ù �� �ǳʶ�

		//�����ü� ���� ����
		while (fgets(tmpData, sizeof(tmpData), fp) != NULL) {
			tmpData[strcspn(tmpData, "\n")] = '\0'; //���๮�� ����

			INFO* new_node = (INFO*)malloc(sizeof(INFO));
			new_node->prev = NULL;
			new_node->next = NULL;
			
			//���� ����
			char* ptr = strtok(tmpData, ",");
			new_node->num = atoi(ptr); //���� ����
			ptr = strtok(NULL, ",");
			strcpy(new_node->name, ptr); //�ü��� ����
			ptr = strtok(NULL, ",");
			strcpy(new_node->address, ptr); //�ּ� ����
			ptr = strtok(NULL, ",");
			strcpy(new_node->note, ptr); //��� ����
			
			//����Ʈ�� ���� �������� ����
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

//����� ���� ���� ���� �Լ�
void UserFileRead() {
	FILE* fp;
	char tmpData[MAX_STR * 5]; //�ӽ� ���ڿ� ���� ����

	fp = fopen("users.txt", "r"); //����� ���� ������ �б� ���� ��
	if (fp == NULL) { //������ ã�� ���Ͽ� NULL�� ��ȯ�� ���
		ErrorHandling("����� ���� ���� Ž���� �����Ͽ����ϴ�!");
	}

	//����� ���� ����
	while (fgets(tmpData, sizeof(tmpData), fp) != NULL) {
		tmpData[strcspn(tmpData, "\n")] = '\0'; //���๮�� ����

		USER* new_node = (USER*)malloc(sizeof(USER));
		new_node->next = NULL;

		char* ptr = strtok(tmpData, "\t");
		strcpy(new_node->id, ptr); //ID ����
		ptr = strtok(NULL, "\t");
		strcpy(new_node->pw, ptr); //PW ����
		ptr = strtok(NULL, "\t");
		new_node->permission = atoi(ptr); //���� ����

		//����Ʈ�� ���� �������� ����
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

//�ü� �ı� ���� ���� ���� �Լ�
void ReviewFileRead() {
	FILE* fp;
	char tmpData[MAX_STR * 5]; //�ӽ� ���ڿ� ���� ����

	fp = fopen("reviews.txt", "r"); //�ü� �ı� ���� ������ �б� ���� ��
	if (fp == NULL) { //������ ã�� ���Ͽ� NULL�� ��ȯ�� ���
		ErrorHandling("�ü� �ı� ���� ���� Ž���� �����Ͽ����ϴ�!");
	}

	//�ü� �ı� ���� ����
	while (fgets(tmpData, sizeof(tmpData), fp) != NULL) {
		tmpData[strcspn(tmpData, "\n")] = '\0'; //���๮�� ����

		REVIEW* new_node = (REVIEW*)malloc(sizeof(REVIEW));
		new_node->next = NULL;

		char* ptr = strtok(tmpData, "\t");

		new_node->division = atoi(ptr); //���� ����
		ptr = strtok(NULL, "\t");
		new_node->num = atoi(ptr); //���� ����
		ptr = strtok(NULL, "\t");
		strcpy(new_node->text, ptr); //�ı� ����

		//����Ʈ�� ���� �������� ����
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

//�����ü� ���� ���� ������Ʈ �Լ�
void InfoFileUpdate(int num) {
	FILE* fp;

	//������ ���� ���� ��
	fp = fopen(all_files[num], "w");
	if (fp == NULL) {
		ErrorHandling("���� Ž���� �����Ͽ����ϴ�!");
	}

	//�ݺ������� ���α׷��� ������ �ִ� ����ü�� ������ ���Ͽ� ���
	INFO* tmp = *(all_heads[num]);
	fprintf(fp, "����,�ü���,�ּ�,���\n");
	while(tmp != NULL) {
		fprintf(fp, "%d,%s,%s,%s\n", tmp->num, tmp->name, tmp->address, tmp->note);
		tmp = tmp->next;
	}

	fclose(fp); //������ ����
}

//����� ���� ���� ������Ʈ �Լ�
void UserFileUpdate() {
	FILE* fp;

	//������ ���� ���� ��
	fp = fopen("users.txt", "w");
	if (fp == NULL) {
		ErrorHandling("���� Ž���� �����Ͽ����ϴ�!");
	}

	//�ݺ������� ���α׷��� ������ �ִ� ����ü�� ������ ���Ͽ� ���
	USER* tmp = user_head;
	while (tmp != NULL) {
		fprintf(fp, "%s\t%s\t%d\n", tmp->id, tmp->pw, tmp->permission);
		tmp = tmp->next;
	}

	fclose(fp); //������ ����
}

//�ü� �ı� ���� ���� ������Ʈ �Լ�
void ReviewFileUpdate() {
	FILE* fp;

	//������ ���� ���� ��
	fp = fopen("reviews.txt", "w");
	if (fp == NULL) {
		ErrorHandling("���� Ž���� �����Ͽ����ϴ�!");
	}

	//�ݺ������� ���α׷��� ������ �ִ� ����ü�� ������ ���Ͽ� ���
	REVIEW* tmp = review_head;
	while (tmp != NULL) {
		fprintf(fp, "%d\t%d\t%s\n", tmp->division, tmp->num, tmp->text);
		tmp = tmp->next;
	}

	fclose(fp); //������ ����
}

//ȸ������ �Լ�
void SignUpProcess(SOCKET clientSock, char* ptr) {
	char sendMsg[MAX_BUF]; //�۽� ���� ����
	char id[MAX_LEN], pw[MAX_LEN];

	ptr = strtok(NULL, "/");
	strcpy(id, ptr); //Ŭ���̾�Ʈ�� �Է��� ID ����
	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); //Ŭ���̾�Ʈ�� �Է��� PW ����

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����

	//ID �ߺ� �˻�
	USER* tmp = user_head;
	while (tmp != NULL) {
		if (strcmp(tmp->id, id) == 0) {
			ReleaseMutex(hMutex); //���ؽ� ����
			sprintf(sendMsg, "Login/new/fail");
			send(clientSock, sendMsg, strlen(sendMsg), 0);
			return;
		}
		tmp = tmp->next;
	}

	//���ο� ��� ����
	USER* new_node = (USER*)malloc(sizeof(USER));

	strcpy(new_node->id, id); //ID ����
	strcpy(new_node->pw, pw); //PW ����
	new_node->permission = 0; //���� �ʱ�ȭ
	new_node->next = NULL;

	//����Ʈ�� ���� �������� ����
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
	UserFileUpdate(); //����� ���� ���� ������Ʈ
	ReleaseMutex(hMutex); //���ؽ� ����

	//ȸ������ ���� ��ȣ �۽�
	sprintf(sendMsg, "Login/new/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
}

//�α��� �Լ�
int LoginProcess(SOCKET clientSock, char* ptr) {
	char sendMsg[MAX_BUF]; //�۽� ���� ����
	char id[MAX_LEN], pw[MAX_LEN];
	int loginCheck = 0; //�α��� ���� ���θ� Ȯ���ϴ� ����

	ptr = strtok(NULL, "/");
	strcpy(id, ptr); //Ŭ���̾�Ʈ�� �Է��� ID ����

	ptr = strtok(NULL, "/");
	strcpy(pw, ptr); //Ŭ���̾�Ʈ�� �Է��� PW ����

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	//Ŭ���̾�Ʈ�� �Է��� ID�� PW�� ����� ���� ���Ḯ��Ʈ�� �����ϴ��� ��ȸ�ϸ� Ž�� 
	USER* tmp = user_head;
	while (tmp != NULL) {
		//ID�� PW�� ��� ��ġ�� ��
		if (strcmp(tmp->id, id) == 0 && strcmp(tmp->pw, pw) == 0) {
			loginCheck = 1; //�α��� ���� üũ
			break;
		}
		tmp = tmp->next;
	}
	ReleaseMutex(hMutex); //���ؽ� ����

	if (loginCheck) {
		sprintf(sendMsg, "Login/member/success/%d", tmp->permission);
		send(clientSock, sendMsg, strlen(sendMsg), 0); //�α��� ���� ��ȣ send
		return 1;
	}
	else {
		sprintf(sendMsg, "Login/member/fail");
		send(clientSock, sendMsg, strlen(sendMsg), 0); //�α��� ���� ��ȣ send
		return 0;
	}
}

//�ü� �ı� �ۼ� �Լ�
void ReviewWrite(SOCKET clientSock, INFO* select_facility, int division, char* ptr) {
	REVIEW* new_node = (REVIEW*)malloc(sizeof(REVIEW));
	time_t rawtime;
	struct tm* timeInfo;

	char sendMsg[MAX_BUF]; //�۽� ���� ����
	char id[MAX_LEN];
	char reviewData[MAX_STR * 10];
	char timeData[MAX_STR]; // �ð��� �����ϴ� ���ڿ�

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	ptr = strtok(NULL, "/");
	strcpy(id, ptr);
	ptr = strtok(NULL, "/");
	strcpy(reviewData, ptr);

	new_node->division = division;
	new_node->num = select_facility->num;
	new_node->next = NULL;

	time(&rawtime); //���� �ð� ����
	timeInfo = localtime(&rawtime); //�ð� ������ ���� �ð��뿡 �°� ��ȯ
	strftime(timeData, sizeof(timeData), "%y/%m/%d %H:%M", timeInfo); //���ϴ� �������� ���ڿ��� ����
	sprintf(new_node->text, "[%s][%s][%s]", id, timeData, reviewData);

	//����Ʈ�� �������� ����
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
	ReleaseMutex(hMutex); //���ؽ� ����

	sprintf(sendMsg, "Info/review/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
}

//�ü� ���� ���� �Լ�
void InfoEdit(SOCKET clientSock, INFO* select_facility, int division, char* ptr) {
	char sendMsg[MAX_BUF]; //�۽� ���� ����
	
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����

	//�ü��� ����
	if (strcmp(ptr, "name") == 0) {
		ptr = strtok(NULL, "/");
		strcpy(select_facility->name, ptr);
	}
	//�ü� �ּ� ����
	else if (strcmp(ptr, "address") == 0) {
		ptr = strtok(NULL, "/");
		strcpy(select_facility->address, ptr);
	}
	//���� ó��
	else{
		
	}

	InfoFileUpdate(division);
	ReleaseMutex(hMutex); //���ؽ� ����

	sprintf(sendMsg, "Info/edit/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
}

//����� ���� ���� �Լ�
void UserEdit(SOCKET clientSock, USER* current_user, char* ptr) {
	char recvMsg[MAX_BUF], sendMsg[MAX_BUF]; //���� ���� �� �۽� ���� ����
	int msgLen = 0; //recv�� �޽��� ������ ũ�⸦ �����ϴ� ����
	
	ptr = strtok(NULL, "/");

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����
	strcpy(current_user->pw, ptr); //���� ��й�ȣ�� ���ο� ��й�ȣ�� ��ü
	UserFileUpdate();
	ReleaseMutex(hMutex); //���ؽ� ����

	sprintf(sendMsg, "User/edit/success");
	send(clientSock, sendMsg, strlen(sendMsg), 0);
	return;
}

//����� ���� ���� �Լ�
void UserDelete(SOCKET clientSock, char* ptr) {
	char sendMsg[MAX_BUF]; //�۽� ���� ����
	char id[MAX_LEN];

	ptr = strtok(NULL, "/");
	strcpy(id, ptr);

	WaitForSingleObject(hMutex, INFINITE); //���ؽ� ����

	//����Ʈ�� ������� ���
	if (user_head == NULL) {
		sprintf(sendMsg, "User/delete/fail"); //���� ��ȣ �۽�
		send(clientSock, sendMsg, strlen(sendMsg), 0);
		ReleaseMutex(hMutex); //���ؽ� ����
		return;
	}

	//����Ʈ�� ��尡 ���� ����� ���
	if (strcmp(user_head->id, id) == 0) {
		//�ش� ��带 ����Ʈ���� ����
		USER* current = user_head;
		user_head = user_head->next;
		free(current);

		sprintf(sendMsg, "User/delete/success"); //���� ��ȣ �۽�
		send(clientSock, sendMsg, strlen(sendMsg), 0);
		UserFileUpdate();
		ReleaseMutex(hMutex); //���ؽ� ����
		return;
	}

	//����Ʈ���� �ش� ID�� ã��
	USER* tmp = user_head;
	while (tmp->next != NULL) {
		if (strcmp(tmp->next->id, id) == 0) {
			//�ش� ��带 ����Ʈ���� ����
			USER* current = tmp->next;
			tmp->next = tmp->next->next;
			free(current);
			
			sprintf(sendMsg, "User/delete/success"); //���� ��ȣ �۽�
			send(clientSock, sendMsg, strlen(sendMsg), 0);
			UserFileUpdate();
			ReleaseMutex(hMutex); //���ؽ� ����
			return;
		}
		tmp = tmp->next;
	}

	ReleaseMutex(hMutex); //���ؽ� ����
}

//�����ü� ���� ����Ʈ �޸� ���� �Լ�
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

//����� ���� ����Ʈ �޸� ���� �Լ�
void UserFree() {
	USER* current = user_head;

	while (current != NULL) {
		USER* next_node = current->next;
		free(current);
		current = next_node;
	}
	user_head = NULL;
}

//�ü� �ı� ���� ����Ʈ �޸� ���� �Լ�
void ReviewFree() {
	REVIEW* current = review_head;

	while (current != NULL) {
		REVIEW* next_node = current->next;
		free(current);
		current = next_node;
	}
	review_head = NULL;
}

//���� ��Ȳ ó�� �Լ�
void ErrorHandling(char* msg) {
	fputs(msg, stderr); //�Ű������� ���� �޽����� ���
	fputc('\n', stderr); //���� ���� ���
	exit(1); //���α׷� ����
}