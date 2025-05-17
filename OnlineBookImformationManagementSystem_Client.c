#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <Windows.h>
#include <process.h>

#define MAX_BUF 8192 //�ִ� ���� ũ��
#define MAX_STR 100 //�ִ� ���ڿ� ����
#define MAX_LEN 20 //�ִ� ID, PW ����

void ErrorHandling(char* msg); //���� ��Ȳ ó�� �Լ�

int main() {
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN serverAddr;

	char serverIp[100] = "127.0.0.1";
	char port[100] = "55555";

	char recvMsg[MAX_BUF], sendMsg[MAX_BUF];
	int msgLen = 0; //recv�� �޽��� ������ ũ�⸦ ����

	//���� IP �ּ� �Է�
	/*printf("Input server IP : ");
	gets(serverIp);*/

	//��Ʈ ��ȣ �Է�
	/*printf("Input server port : ");
	gets(port);*/

	//���� ��� ����
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	//Ŭ���̾�Ʈ ���� ����
	sock = socket(PF_INET, SOCK_STREAM, 0);

	//���� �ּ� ����
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIp);
	serverAddr.sin_port = htons(atoi(port));

	//���� ����
	if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	//�α��� ó��
	printf("�������������ý��ۿ� ���� ���� ȯ���մϴ�!\n");
	while (1) {
		char id[MAX_LEN], pw[MAX_LEN];
		int loginCheck = 0; //�α��� ���� ���θ� Ȯ���ϴ� �Լ�
		
		printf("ID�� �Է����ּ���: ");
		gets(id);
		printf("PW�� �Է����ּ���: ");
		
		int i = 0;
		while (1) {
			pw[i] = _getch();
		
			if (pw[i] == '\r') { //getch�� ���� �Է��� \r�� \n �� �ܰ迡 ���� ó����
				pw[i] = '\0';
				printf("\n");
				break;
			}
			printf("*"); //getch �Լ��� �Է��� ȭ�鿡 ��µ��� �����Ƿ� ��� * ���
			i++;
		}
		
		//ID�� PW�� ������ �α��� ��ȣ send
		sprintf(sendMsg, "LOGIN/0/%s/%s", id, pw);
		send(sock, sendMsg, strlen(sendMsg), 0);

		//�α��� ��� �޽��� recv
		while ((msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0)) != 0) {
			recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����
			char* ptr = strtok(recvMsg, "/");

			//recv�� �޽����� �α��� ��ȣ��
			if (strcmp(ptr, "LOGIN") == 0) {
				ptr = strtok(NULL, "/");

				//�α��� ��ȯ ��ȣ��
				if (strcmp(ptr, "1") == 0) {
					ptr = strtok(NULL, "/");

					//�α��� ���� ��
					if (strcmp(ptr, "SUCCESS") == 0) {
						printf("\n�α��� ����!\n");
						loginCheck = 1;
					}
					//�α��� ���� ��
					else if (strcmp(ptr, "FAIL") == 0) {
						printf("\n�α��� ����.. ID�� PW�� �ٽ� �Է����ּ���.\n");
					}
					break;
				}
			}
		}
		//�α��� ���� �� �ݺ��� ����
		if (loginCheck) {
			break;
		}
	}
	
	printf("\n-----------------------------\n");
	printf("-----���������ý����Դϴ�----\n");
	printf("-----------------------------\n");

	while (1) {
		int command;
		char inputMsg[MAX_STR];

		printf("\n1. ���� ���� ���� 2. ����� ���� ���� 3. ���α׷� ����\n�Է�: ");
		scanf("%d", &command); //����� ����� ��ȣ�� �Է¹���
		getchar(); //���๮�ڸ� ���ۿ��� ���

		//���� ���� ����
		if (command == 1) {
			printf("\n1. ���� ���� �˻� 2. ���� ���� �߰� 3. ���� ��ŷ\n�Է�: ");
			scanf("%d", &command); //����� ����� ��ȣ�� �Է¹���
			getchar(); //���๮�ڸ� ���ۿ��� ���

			//���� �˻�
			if (command == 1) {
				printf("\n�˻��� �������� �Է����ּ���: ");
				gets(inputMsg);
				sprintf(sendMsg, "Book/Search/%s", inputMsg);
				send(sock, sendMsg, strlen(sendMsg), 0); //�˻��� �������� ��� �޽����� ������ send

				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0); //���� ��ȣ�� recv
				recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

				char* ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "Book") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "Search") == 0) {
						ptr = strtok(NULL, "/");

						//���� �˻� ���� ��
						if (strcmp(ptr, "1") == 0) {
							ptr = strtok(NULL, "/");
							printf("%s", ptr); //�����κ��� recv ���� ���� ������ ���

							printf("\n1. ���� ���� ���� 2. ���� ���� ���� 3. ���\n�Է�: ");
							scanf("%d", &command); //����� ����� ��ȣ�� �Է¹���
							getchar(); //���๮�ڸ� ���ۿ��� ���

							//���� ����
							if (command == 1) {
								printf("������ �׸��� �Է����ּ���!(1. ������, 2. ����, 3. ����)\n�Է�: ");
								scanf("%d", &command); //����� ����� ��ȣ�� �Է¹���
								getchar(); //���๮�ڸ� ���ۿ��� ���

								printf("���� ���� �Է�: ");
								gets(inputMsg);

								sprintf(sendMsg, "Book/Edit/%d/%s", command, inputMsg);
								send(sock, sendMsg, strlen(sendMsg), 0); //������ ������ ���� ���� �׸�� ������ ��� �޽����� send
								printf("���� ������ ����Ǿ����ϴ�!\n");
							}
							//���� ����
							else if (command == 2) {
								printf("������ �����Ͻðڽ��ϱ�? (Y / N): ");
								scanf("%c", &command);

								if (command == 'Y' || command == 'y') { //Y �Ǵ� y�� �Է¹�����
									sprintf(sendMsg, "Book/Delete/1");
									send(sock, sendMsg, strlen(sendMsg), 0); //������ ���� ���� ���� ��ȣ�� send
									printf("������ �����߽��ϴ�.\n");
								}
								else if (command == 'N' || command == 'n') { //N�Ǵ� n�� �Է¹�����
									sprintf(sendMsg, "Book/Delete/0");
									send(sock, sendMsg, strlen(sendMsg), 0); //������ ���� ���� ���� ��� ��ȣ�� send
									printf("���� ������ ����߽��ϴ�.\n");
									continue;
								}
							}
							//���
							else if (command == 3) {
								printf("���� �޴��� �̵��մϴ�.\n");
								continue;
							}
							else {
								continue;
							}
						}
						//���� �˻� ���� ��
						else if (strcmp(ptr, "0") == 0) {
							printf("������ ã�� ���߽��ϴ�!\n");
							continue;
						}
					}
				}
			}
			//���� �߰�
			else if (command == 2) {
				char tempTitle[MAX_STR], tempAuthor[MAX_STR];
				float tempRating;

				printf("�߰��� �������� �Է����ּ���: ");
				gets(tempTitle); //�߰��� �������� �Է¹���

				printf("�߰��� ���ڸ� �Է����ּ���: ");
				gets(tempAuthor); //�߰��� ���ڸ��� �Է¹���

				printf("�߰��� ������ �Է����ּ���: ");
				scanf("%f", &tempRating); //�߰��� ������ �Է¹���

				sprintf(sendMsg, "Book/Add/%s/%s/%.2f", tempTitle, tempAuthor, tempRating); //������ �߰��� ������, ���ڸ�, ������ ��� �޽����� send
				send(sock, sendMsg, strlen(sendMsg), 0);
				printf("���� ������ �߰��Ǿ����ϴ�!\n");
			}
			//���� ��ŷ
			else if (command == 3) {
				sprintf(sendMsg, "Book/Ranking/1");
				send(sock, sendMsg, strlen(sendMsg), 0); //������ ���� ���� ��ŷ ��û ��ȣ�� send

				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0); //���� ���� ��ŷ �����͸� recv ����
				recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����
				
				printf("\n[���� ���� 30���� ���� �� ��ŷ�Դϴ�.]\n\n");
				printf("%s", recvMsg);
			}
			//���� ó��
			else {
				continue;
			}
		}
		//����� ���� ����
		else if (command == 2) {
			char id[20], pw[20], newPw[20];

			printf("\n1. ����� ���� ����, 2. ����� ���� ����, 3. ����� ���� �߰�\n�Է�: ");
			scanf("%d", &command); //����� ����� ��ȣ�� �Է¹���
			getchar(); //���๮�ڸ� ���ۿ��� ���

			//����� ���� ����
			if (command == 1) {
				printf("������ ID�� �Է����ּ���: ");
				gets(id);

				printf("�ش� ID�� PW�� �Է����ּ���: ");
				gets(pw);

				printf("���ο� PW�� �Է����ּ���: ");
				gets(newPw); //���Ӱ� �ٲ� PW�� �Է¹���

				sprintf(sendMsg, "User/Edit/%s/%s/%s", id, pw, newPw);
				send(sock, sendMsg, strlen(sendMsg), 0); //������ ������ ID�� �ش� ID�� PW, ���Ӱ� �ٲ� PW�� ��� �޽����� send

				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0); //���� ��ȣ�� recv
				recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

				char* ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "User") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "Edit") == 0) {
						ptr = strtok(NULL, "/");

						//����� ���� ���濡 ������ ���
						if (strcmp(ptr, "1") == 0) {
							printf("���濡 �����߽��ϴ�!\n");
						}
						//����� ���� ���濡 ������ ���
						else if (strcmp(ptr, "0") == 0) {
							printf("���濡 �����߽��ϴ�.. �ùٸ� ID �Ǵ� PW�� �Է����ּ���.\n");
						}
					}
				}
			}
			//����� ���� ����
			else if (command == 2) {
				printf("������ ID�� �Է����ּ���: ");
				gets(id);

				printf("���� �����Ͻðڽ��ϱ�? (Y / N): ");
				scanf("%c", &command);

				if (command == 'Y' || command == 'y') { //Y �Ǵ� y�� �Է¹�����
					sprintf(sendMsg, "User/Delete/1/%s", id);
					send(sock, sendMsg, strlen(sendMsg), 0); //������ ����� ���� ���� ��ȣ�� send
					printf("����� ������ �����߽��ϴ�.\n");
				}
				else if (command == 'N' || command == 'n') { //N �Ǵ� n�� �Է¹�����
					sprintf(sendMsg, "User/Delete/0");
					send(sock, sendMsg, strlen(sendMsg), 0); //������ ����� ���� ���� ��� ��ȣ�� send
					printf("����� ���� ������ ����߽��ϴ�.\n");
					continue;
				}
			}
			//����� ���� �߰�
			else if (command == 3) {
				printf("�߰��� ID�� �Է����ּ���: ");
				gets(id); //�߰��� ID�� �Է¹���

				printf("�߰��� PW�� �Է����ּ���: ");
				gets(pw); //�߰��� PW�� �Է¹���

				sprintf(sendMsg, "User/Add/%s/%s", id, pw);
				send(sock, sendMsg, strlen(sendMsg), 0); //������ �߰��� ID�� PW�� ��� �޽����� send

				printf("����� ������ �߰��Ǿ����ϴ�!\n");
			}
		}
		//���α׷� ����
		else if (command == 3) {
			printf("���α׷��� �����մϴ�.\n");
			send(sock, "EXIT", 4, 0); // ������ ���α׷� ���� ��ȣ�� send
			break; //��� ���� �ݺ��� ����
		}
		//���� ó��
		else {
			printf("�߸��� �Է��Դϴ�.\n");
		}
	}

	//���� ���� ó��
	closesocket(sock);
	WSACleanup();
	
	//���α׷� ����
	return 0;
}

//���� ��Ȳ ó�� �Լ�
void ErrorHandling(char* msg) {
	fputs(msg, stderr); //�Ű������� ���� �޽����� ���
	fputc('\n', stderr); //���� ���� ���
	exit(1); //���α׷� ����
}