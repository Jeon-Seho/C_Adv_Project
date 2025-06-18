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

	char recvMsg[MAX_BUF], sendMsg[MAX_BUF]; //���� �� �۽� �޽��� ����
	int msgLen = 0; //���Ź��� �޽��� ������ ũ��
	char id[MAX_LEN], pw[MAX_LEN]; //ID�� PW 
	int loginCheck = 0; //�α��� ���� ����
	int permission = 0; //����

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
	printf("�ƻ�� �����ü� �ȳ� ���񽺿� ���� ���� ȯ���մϴ�!\n");
	while (1) {
		char command[10];

		printf("\n1. ȸ������ 2. ȸ�� �α��� 3. ��ȸ�� �α���\n�Է�: ");
		gets(command);

		//ȸ������
		if (strcmp(command, "1") == 0) {
			char checkPw[MAX_LEN]; //��й�ȣ Ȯ��

			printf("ID�� �Է����ּ���(19�� �̳�): ");
			gets(id);
			printf("PW�� �Է����ּ���(19�� �̳�): ");

			int i = 0;
			while (i < MAX_LEN - 1) {
				pw[i] = _getch();

				if (pw[i] == '\r') { //getch�� ���� �Է��� \r�� \n �� �ܰ迡 ���� ó����
					pw[i] = '\0';
					printf("\n");
					break;
				}
				printf("*"); //getch �Լ��� �Է��� ȭ�鿡 ��µ��� �����Ƿ� ��� * ���
				i++;
			}
			
			//��й�ȣ ��Ȯ��
			printf("PW�� �� �� �� �Է����ּ���: ");
			i = 0;
			while (i < MAX_LEN - 1) {
				checkPw[i] = _getch();

				if (checkPw[i] == '\r') { //getch�� ���� �Է��� \r�� \n �� �ܰ迡 ���� ó����
					checkPw[i] = '\0';
					printf("\n");
					break;
				}
				printf("*"); //getch �Լ��� �Է��� ȭ�鿡 ��µ��� �����Ƿ� ��� * ���
				i++;
			}

			//��й�ȣ�� �ٸ��ٸ�
			if (strcmp(pw, checkPw) != 0) {
				printf("\n��й�ȣ�� �߸� �ԷµǾ����ϴ�. ȸ�������� �ٽ� �õ����ּ���.\n");
				continue;
			}

			//������ ID�� PW�� ������ ȸ������ ��ȣ �۽�
			sprintf(sendMsg, "Login/new/%s/%s", id, pw);
			send(sock, sendMsg, strlen(sendMsg), 0);

			//������ ���� �޽����� ����
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0';

			char* ptr = strtok(recvMsg, "/");

			if (strcmp(ptr, "Login") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "new") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "success") == 0) {
						printf("\nȸ�����Կ� �����߽��ϴ�! �α������ּ���.\n");
						continue;
					}
					else if (strcmp(ptr, "fail") == 0) {
						printf("\nȸ�����Կ� �����߽��ϴ�.. ID�� �������ּ���.\n");
						continue;
					}
				}
			}
		}
		//ȸ�� �α���
		else if (strcmp(command, "2") == 0) {
			
			printf("ID�� �Է����ּ���: ");
			gets(id);
			printf("PW�� �Է����ּ���: ");

			int i = 0;
			while (i < MAX_LEN - 1) {
				pw[i] = _getch();

				if (pw[i] == '\r') { //getch�� ���� �Է��� \r�� \n �� �ܰ迡 ���� ó����
					pw[i] = '\0';
					printf("\n");
					break;
				}
				printf("*"); //getch �Լ��� �Է��� ȭ�鿡 ��µ��� �����Ƿ� ��� * ���
				i++;
			}

			//������ ID�� PW�� ������ �α��� ��ȣ �۽�
			sprintf(sendMsg, "Login/member/%s/%s", id, pw);
			send(sock, sendMsg, strlen(sendMsg), 0);

			//������ ���� �޽����� ����
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����
			
			char* ptr = strtok(recvMsg, "/");

			//���Ź��� �޽����� �α��� ��ȣ��
			if (strcmp(ptr, "Login") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "member") == 0) {
					ptr = strtok(NULL, "/");

					//�α��� ���� ��
					if (strcmp(ptr, "success") == 0) {
						printf("\n�α��� ����!\n");
						loginCheck = 1;

						//���� ����
						ptr = strtok(NULL, "/");
						permission = atoi(ptr);
					}
					//�α��� ���� ��
					else if (strcmp(ptr, "fail") == 0) {
						printf("\n�α��� ����.. �ٽ� �α������ּ���.\n");
					}
				}
			}
			
			//�α��� ���� �� �ݺ��� ����
			if (loginCheck) {
				break;
			}
		}
		//��ȸ�� �α���
		else if (strcmp(command, "3") == 0) {
			printf("\n��ȸ�� �α����� �õ��մϴ�...\n");

			//������ ��ȸ�� �α��� ��ȣ �۽�
			sprintf(sendMsg, "Login/guest");
			send(sock, sendMsg, strlen(sendMsg), 0);

			//������ ���� �޽����� ����
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0'; //recv�� �޽��� ������ �������� NULL���� ����

			char* ptr = strtok(recvMsg, "/");

			//���Ź��� �޽����� �α��� ��ȣ��
			if (strcmp(ptr, "Login") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "guest") == 0) {
					ptr = strtok(NULL, "/");
					
					if (strcmp(ptr, "success") == 0) {
						printf("\n��ȸ�� �α��ο� �����߽��ϴ�!\n");

						strcpy(id, "guest");
						strcpy(pw, "guest");

						break;
					}
				}
			}

			printf("\n��ȸ�� �α��ο� �����߽��ϴ�.\n");
		}
		//���� ó��
		else {
			printf("\n�߸��� �Է��Դϴ�.\n");
			continue;
		}
	}

	printf("\n------------------------------------\n");
	printf("-----�ƻ�� �����ü� �ȳ� ����----\n");
	printf("------------------------------------\n");

	//��� ����
	while (1) {
		char command[10];
		int division = -1;
		char tmpMsg[MAX_STR];
		
		printf("\n1. �����ü� �˻� 2. ����� ���� ���� 3. ���α׷� ����\n�Է�: ");
		gets(command);

		//�����ü� �˻�
		if (strcmp(command, "1") == 0) {
			printf("\n���� ����: 1. ����νü� 2. ��ȸ������ü 3. ���κ����ü� 4. ������ 5. ���� 6. ����ȸ��\n�Է�: ");
			scanf(" %d", &division);
			getchar();

			//������ ������ �����ü� ���� �۽�
			sprintf(sendMsg, "Info/division/%d", division);
			send(sock, sendMsg, strlen(sendMsg), 0);

			//��� ����
			while (1) {
				//������ �ü��� ��� ���� �޽��� ����
				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
				recvMsg[msgLen] = '\0';

				//�ü��� ��� ���
				char* ptr = strtok(recvMsg, "/");
				printf("------------------------------------------------------------------------------------------------------------------------\n");
				printf("%s\n", ptr);
				printf("------------------------------------------------------------------------------------------------------------------------\n");

				//ó�� �Ǵ� ������ ������ ��ȣ Ȯ��
				ptr = strtok(NULL, "/");
				if (strcmp(ptr, "start") == 0) {
					printf("<ù ��° �������Դϴ�.>\n");
				}
				else if (strcmp(ptr, "end") == 0) {
					printf("<������ �������Դϴ�.>\n");
				}

				printf("\n1. �ü� ���� 2. �ּ� �˻� 3. ���� ������ 4. ���� ������\n�Է�: ");
				gets(command);

				//�ü��� �Է�
				if (strcmp(command, "1") == 0) {
					printf("�ü��� �Է�: ");
					gets(tmpMsg);

					//������ ������ �ü��� �۽�
					sprintf(sendMsg, "Info/select/%s", tmpMsg);
					send(sock, sendMsg, strlen(sendMsg), 0);

					break;
				}
				//�ּ� �˻�
				else if (strcmp(command, "2") == 0) {
					printf("�ּ� �Է�: ");
					gets(tmpMsg);

					//������ �˻��ϰ��� �ϴ� �ּ� �۽�
					sprintf(sendMsg, "Info/search/%s", tmpMsg);
					send(sock, sendMsg, strlen(sendMsg), 0);

					//������ ���� �޽��� ����
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0';

					//�˻��� ������ ���
					if (strcmp(recvMsg, "Info/search/fail") == 0) {
						printf("�������� �ʴ� �ּ��Դϴ�. �ٽ� �Է����ּ���.\n");
						continue;
					}
					//�˻��� ������ ���
					else {
						printf("%s\n", recvMsg); //�ü��� ��� ���

						printf("�ü��� �Է�: ");
						gets(tmpMsg);

						//������ ������ �ü��� �۽�
						sprintf(sendMsg, "Info/select/%s", tmpMsg);
						send(sock, sendMsg, strlen(sendMsg), 0);

						break;
					}
				}
				//���� ������
				else if (strcmp(command, "3") == 0) {
					//������ ���� ������ ��ȣ �۽�
					sprintf(sendMsg, "Info/prev");
					send(sock, sendMsg, strlen(sendMsg), 0);
				}
				//���� ������
				else if (strcmp(command, "4") == 0) {
					//������ ���� ������ ��ȣ �۽�
					sprintf(sendMsg, "Info/next");
					send(sock, sendMsg, strlen(sendMsg), 0);
				}
				//���� ó��
				else {
					printf("�߸��� �Է��Դϴ�.\n");
					continue;
				}
			}
			
			//������ ���� �ü� �� ���� ����
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0';

			//�ü��� ���� ���� Ȯ��
			char* ptr = strtok(recvMsg, "/");

			if (strcmp(ptr, "Info") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "select") == 0) {
					ptr = strtok(NULL, "/");

					//�������� �ʴ� �ü����� ���
					if (strcmp(ptr, "fail") == 0) {
						printf("�������� �ʴ� �ü����Դϴ�. �ٽ� �Է����ּ���.\n");
						continue;
					}
					//�����ϴ� �ü����� ���
					else {
						printf("\n%s\n", ptr); //�ü� �� ���� ���
					}
				}
			}

			//������ �ü� �ı� ������ ����
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0';

			//�ü� �ı� ���� ���
			printf("\n----- Review -----\n");
			printf("%s", recvMsg);

			printf("\n1. �ı� �ۼ� 2. �ü� ���� ���� 3. ����\n�Է�: ");
			gets(command);

			//�ı� �ۼ�
			if (strcmp(command, "1") == 0) {
				//�α��� ���� Ȯ��
				if (loginCheck) {
					printf("�ı� �Է�: ");
					gets(tmpMsg);

					//������ ID�� �ۼ��� �ı� �۽�
					sprintf(sendMsg, "Info/review/%s/%s", id, tmpMsg);
					send(sock, sendMsg, strlen(sendMsg), 0);

					//������ ���� �޽��� ����
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0';

					ptr = strtok(recvMsg, "/");

					if (strcmp(ptr, "Info") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "review") == 0) {
							ptr = strtok(NULL, "/");

							//�ı� �ۼ��� ������ ���
							if (strcmp(ptr, "success") == 0) {
								printf("�ı� �ۼ��� �����߽��ϴ�!\n");
								continue;
							}
						}
					}

					printf("�ı� �ۼ��� �����߽��ϴ�..\n");
					continue;
				}
				//��ȸ���� ���
				else {
					printf("ȸ������ �Ǵ� �α������ּ���.\n");
					continue;
				}
			}
			//�ü� ���� ����
			else if (strcmp(command, "2") == 0) {
				//���� �˻�
				if (permission == 1) {
					printf("\n1. �ü��� ���� 2. �ü� �ּ� ����\n�Է�: ");
					gets(command);

					//�ü��� ����
					if (strcmp(command, "1") == 0) {
						printf("���ο� �ü����� �Է����ּ���: ");
						gets(tmpMsg);

						//������ �����ϰ����ϴ� �ü��� �۽�
						sprintf(sendMsg, "Info/edit/name/%s", tmpMsg);
						send(sock, sendMsg, strlen(sendMsg), 0);
					}
					//�ü� �ּ� ����
					else if (strcmp(command, "2") == 0) {
						printf("���ο� �ּҸ� �Է����ּ���: ");
						gets(tmpMsg);

						//������ �����ϰ����ϴ� �ּ� �۽�
						sprintf(sendMsg, "Info/edit/address/%s", tmpMsg);
						send(sock, sendMsg, strlen(sendMsg), 0);
					}
					//���� ó��
					else {
						printf("�߸��� �Է��Դϴ�.\n");
						continue;
					}

					//������ ���� �޽��� ���� 
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0';

					ptr = strtok(recvMsg, "/");

					if (strcmp(ptr, "Info") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "edit") == 0) {
							ptr = strtok(NULL, "/");

							//���� ������ ������ ���
							if (strcmp(ptr, "success") == 0) {
								printf("���� ������ �����߽��ϴ�!\n");
								continue;
							}
						}
					}

					printf("���� ������ �����߽��ϴ�..\n");
					continue;
				}
				else {
					printf("�ü� ������ ������ ������ �����ϴ�.\n");
					continue;
				}
			}
			//����
			else if (strcmp(command, "3") == 0) {
				printf("ó������ ���ư��ϴ�.\n");
				sprintf(sendMsg, "Info/end");
				send(sock, sendMsg, strlen(sendMsg), 0);

				continue;
			}
			//���� ó��
			else {
				printf("�߸��� �Է��Դϴ�.\n");
				continue;
			}
		}
		//����� ���� ����
		else if (strcmp(command, "2") == 0) {
			//�α��� ���� Ȯ��
			if (loginCheck) {
				//��й�ȣ Ȯ��
				printf("���� �α��� ���� ������ PW�� �Է����ּ���: ");
				gets(pw);

				//������ ID�� PW�� �۽�
				sprintf(sendMsg, "User/checkpw/%s/%s", id, pw);
				send(sock, sendMsg, strlen(sendMsg), 0);

				//������ ���� �޽����� ����
				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
				recvMsg[msgLen] = '\0';

				//��й�ȣ ��ġ ���� Ȯ��
				char* ptr = strtok(recvMsg, "/");

				if (strcmp(ptr, "User") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "checkpw") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "success") == 0) {
							printf("��й�ȣ�� ��ġ�մϴ�. ���� �޴��� �̵��մϴ�.\n");
						}
						else if (strcmp(ptr, "fail") == 0) {
							printf("��й�ȣ�� �ٽ� Ȯ�����ּ���!\n");
							continue;
						}
					}
				}

				printf("\n1. ����� ���� ���� 2. ����� ���� ����\n�Է�: ");
				gets(command);

				//����� ���� ����
				if (strcmp(command, "1") == 0) {
					char newPw[MAX_LEN];
					printf("���ο� ��й�ȣ�� �Է����ּ���: ");
					gets(newPw);

					//������ ID�� ���ο� PW�� �۽�
					sprintf(sendMsg, "User/edit/%s", newPw);
					send(sock, sendMsg, strlen(sendMsg), 0);

					//������ ���� �޽����� ����
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0'; 

					ptr = strtok(recvMsg, "/");

					if (strcmp(ptr, "User") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "edit") == 0) {
							ptr = strtok(NULL, "/");

							//����� ���� ������ ������ ���
							if (strcmp(ptr, "success") == 0) {
								printf("��й�ȣ ���濡 �����߽��ϴ�!\n");
							}
							//����� ���� ������ ������ ���
							else if (strcmp(ptr, "fail") == 0) {
								printf("��й�ȣ ���濡 �����߽��ϴ�..\n");
							}
						}
					}
				}
				//����� ���� ����
				else if (strcmp(command, "2") == 0) {
					printf("������ ����� ������ �����Ͻðڽ��ϱ�? (Y / N): ");
					gets(command);

					//Y �Ǵ� y�� �Է��� ���
					if (strcmp(command, "Y") == 0 || strcmp(command, "y") == 0) {
						//������ ������ ID�� �۽�
						sprintf(sendMsg, "User/delete/%s", id);
						send(sock, sendMsg, strlen(sendMsg), 0);

						//������ ���� �޽����� ����
						msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
						recvMsg[msgLen] = '\0';

						ptr = strtok(recvMsg, "/");

						if (strcmp(ptr, "User") == 0) {
							ptr = strtok(NULL, "/");

							if (strcmp(ptr, "delete") == 0) {
								ptr = strtok(NULL, "/");

								//����� ���� ������ ������ ���
								if (strcmp(ptr, "success") == 0) {
									printf("����� ���� ������ �����߽��ϴ�!\n");
									break;
								}
								//����� ���� ������ ������ ���
								else if (strcmp(ptr, "fail") == 0) {
									printf("����� ���� ������ �����߽��ϴ�..\n");
									continue;
								}
							}
						}
					}
					//N �Ǵ� n�� �Է��� ���
					else if (strcmp(command, "N") == 0 || strcmp(command, "n") == 0) {
						printf("����� ���� ������ ����մϴ�.\n");
						continue;
					}
					//���� ó��
					else {
						printf("�߸��� �Է��Դϴ�.\n");
						continue;
					}
				}
				//���� ó��
				else {
					printf("�߸��� �Է��Դϴ�.\n");
					continue;
				}
			}
			//��ȸ���� ���
			else {
				printf("ȸ������ �Ǵ� �α������ּ���.\n");
				continue;
			}
		}
		//���α׷� ����
		else if (strcmp(command, "3") == 0) {
			printf("���α׷��� �����մϴ�.\n");
			send(sock, "Exit", 4, 0); // ������ ���α׷� ���� ��ȣ�� send
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