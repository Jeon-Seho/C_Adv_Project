#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <Windows.h>
#include <process.h>

#define MAX_BUF 8192 //최대 버퍼 크기
#define MAX_STR 100 //최대 문자열 길이
#define MAX_LEN 20 //최대 ID, PW 길이

void ErrorHandling(char* msg); //오류 상황 처리 함수

int main() {
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN serverAddr;

	char serverIp[100] = "127.0.0.1";
	char port[100] = "55555";

	char recvMsg[MAX_BUF], sendMsg[MAX_BUF];
	int msgLen = 0; //recv한 메시지 버퍼의 크기를 저장

	//서버 IP 주소 입력
	/*printf("Input server IP : ");
	gets(serverIp);*/

	//포트 번호 입력
	/*printf("Input server port : ");
	gets(port);*/

	//소켓 사용 선언
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	//클라이언트 소켓 생성
	sock = socket(PF_INET, SOCK_STREAM, 0);

	//서버 주소 저장
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIp);
	serverAddr.sin_port = htons(atoi(port));

	//서버 접속
	if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	//로그인 처리
	printf("도서정보관리시스템에 오신 것을 환영합니다!\n");
	while (1) {
		char id[MAX_LEN], pw[MAX_LEN];
		int loginCheck = 0; //로그인 성공 여부를 확인하는 함수
		
		printf("ID를 입력해주세요: ");
		gets(id);
		printf("PW를 입력해주세요: ");
		
		int i = 0;
		while (1) {
			pw[i] = _getch();
		
			if (pw[i] == '\r') { //getch은 엔터 입력을 \r과 \n 두 단계에 걸쳐 처리함
				pw[i] = '\0';
				printf("\n");
				break;
			}
			printf("*"); //getch 함수는 입력이 화면에 출력되지 않으므로 대신 * 출력
			i++;
		}
		
		//ID와 PW를 포함한 로그인 신호 send
		sprintf(sendMsg, "LOGIN/0/%s/%s", id, pw);
		send(sock, sendMsg, strlen(sendMsg), 0);

		//로그인 결과 메시지 recv
		while ((msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0)) != 0) {
			recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입
			char* ptr = strtok(recvMsg, "/");

			//recv한 메시지가 로그인 신호면
			if (strcmp(ptr, "LOGIN") == 0) {
				ptr = strtok(NULL, "/");

				//로그인 반환 신호면
				if (strcmp(ptr, "1") == 0) {
					ptr = strtok(NULL, "/");

					//로그인 성공 시
					if (strcmp(ptr, "SUCCESS") == 0) {
						printf("\n로그인 성공!\n");
						loginCheck = 1;
					}
					//로그인 실패 시
					else if (strcmp(ptr, "FAIL") == 0) {
						printf("\n로그인 실패.. ID와 PW를 다시 입력해주세요.\n");
					}
					break;
				}
			}
		}
		//로그인 성공 시 반복문 종료
		if (loginCheck) {
			break;
		}
	}
	
	printf("\n-----------------------------\n");
	printf("-----도서관리시스템입니다----\n");
	printf("-----------------------------\n");

	while (1) {
		int command;
		char inputMsg[MAX_STR];

		printf("\n1. 도서 정보 관리 2. 사용자 정보 관리 3. 프로그램 종료\n입력: ");
		scanf("%d", &command); //사용할 기능을 번호로 입력받음
		getchar(); //개행문자를 버퍼에서 비움

		//도서 정보 관리
		if (command == 1) {
			printf("\n1. 도서 정보 검색 2. 도서 정보 추가 3. 도서 랭킹\n입력: ");
			scanf("%d", &command); //사용할 기능을 번호로 입력받음
			getchar(); //개행문자를 버퍼에서 비움

			//도서 검색
			if (command == 1) {
				printf("\n검색할 도서명을 입력해주세요: ");
				gets(inputMsg);
				sprintf(sendMsg, "Book/Search/%s", inputMsg);
				send(sock, sendMsg, strlen(sendMsg), 0); //검색할 도서명이 담긴 메시지를 서버로 send

				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0); //서버 신호를 recv
				recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

				char* ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "Book") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "Search") == 0) {
						ptr = strtok(NULL, "/");

						//도서 검색 성공 시
						if (strcmp(ptr, "1") == 0) {
							ptr = strtok(NULL, "/");
							printf("%s", ptr); //서버로부터 recv 받은 도서 정보를 출력

							printf("\n1. 도서 정보 변경 2. 도서 정보 삭제 3. 취소\n입력: ");
							scanf("%d", &command); //사용할 기능을 번호로 입력받음
							getchar(); //개행문자를 버퍼에서 비움

							//도서 변경
							if (command == 1) {
								printf("변경할 항목을 입력해주세요!(1. 도서명, 2. 저자, 3. 평점)\n입력: ");
								scanf("%d", &command); //사용할 기능을 번호로 입력받음
								getchar(); //개행문자를 버퍼에서 비움

								printf("변경 내용 입력: ");
								gets(inputMsg);

								sprintf(sendMsg, "Book/Edit/%d/%s", command, inputMsg);
								send(sock, sendMsg, strlen(sendMsg), 0); //서버로 변경할 도서 정보 항목과 내용이 담긴 메시지를 send
								printf("도서 정보가 변경되었습니다!\n");
							}
							//도서 삭제
							else if (command == 2) {
								printf("정말로 삭제하시겠습니까? (Y / N): ");
								scanf("%c", &command);

								if (command == 'Y' || command == 'y') { //Y 또는 y를 입력받으면
									sprintf(sendMsg, "Book/Delete/1");
									send(sock, sendMsg, strlen(sendMsg), 0); //서버로 도서 정보 삭제 신호를 send
									printf("도서를 삭제했습니다.\n");
								}
								else if (command == 'N' || command == 'n') { //N또는 n을 입력받으면
									sprintf(sendMsg, "Book/Delete/0");
									send(sock, sendMsg, strlen(sendMsg), 0); //서버로 도서 정보 삭제 취소 신호를 send
									printf("도서 삭제를 취소했습니다.\n");
									continue;
								}
							}
							//취소
							else if (command == 3) {
								printf("메인 메뉴로 이동합니다.\n");
								continue;
							}
							else {
								continue;
							}
						}
						//도서 검색 실패 시
						else if (strcmp(ptr, "0") == 0) {
							printf("도서를 찾지 못했습니다!\n");
							continue;
						}
					}
				}
			}
			//도서 추가
			else if (command == 2) {
				char tempTitle[MAX_STR], tempAuthor[MAX_STR];
				float tempRating;

				printf("추가할 도서명을 입력해주세요: ");
				gets(tempTitle); //추가할 도서명을 입력받음

				printf("추가할 저자를 입력해주세요: ");
				gets(tempAuthor); //추가할 저자명을 입력받음

				printf("추가할 평점을 입력해주세요: ");
				scanf("%f", &tempRating); //추가할 평점을 입력받음

				sprintf(sendMsg, "Book/Add/%s/%s/%.2f", tempTitle, tempAuthor, tempRating); //서버로 추가할 도서명, 저자명, 평점이 담긴 메시지를 send
				send(sock, sendMsg, strlen(sendMsg), 0);
				printf("도서 정보가 추가되었습니다!\n");
			}
			//도서 랭킹
			else if (command == 3) {
				sprintf(sendMsg, "Book/Ranking/1");
				send(sock, sendMsg, strlen(sendMsg), 0); //서버로 도서 평점 랭킹 요청 신호를 send

				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0); //도서 평점 랭킹 데이터를 recv 받음
				recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입
				
				printf("\n[상위 도서 30권의 평점 순 랭킹입니다.]\n\n");
				printf("%s", recvMsg);
			}
			//예외 처리
			else {
				continue;
			}
		}
		//사용자 정보 관리
		else if (command == 2) {
			char id[20], pw[20], newPw[20];

			printf("\n1. 사용자 정보 변경, 2. 사용자 정보 삭제, 3. 사용자 정보 추가\n입력: ");
			scanf("%d", &command); //사용할 기능을 번호로 입력받음
			getchar(); //개행문자를 버퍼에서 비움

			//사용자 정보 변경
			if (command == 1) {
				printf("변경할 ID를 입력해주세요: ");
				gets(id);

				printf("해당 ID의 PW를 입력해주세요: ");
				gets(pw);

				printf("새로운 PW를 입력해주세요: ");
				gets(newPw); //새롭게 바꿀 PW를 입력받음

				sprintf(sendMsg, "User/Edit/%s/%s/%s", id, pw, newPw);
				send(sock, sendMsg, strlen(sendMsg), 0); //서버로 변경할 ID와 해당 ID의 PW, 새롭게 바꿀 PW가 담긴 메시지를 send

				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0); //서버 신호를 recv
				recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

				char* ptr = strtok(recvMsg, "/");
				if (strcmp(ptr, "User") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "Edit") == 0) {
						ptr = strtok(NULL, "/");

						//사용자 정보 변경에 성공한 경우
						if (strcmp(ptr, "1") == 0) {
							printf("변경에 성공했습니다!\n");
						}
						//사용자 정보 변경에 실패한 경우
						else if (strcmp(ptr, "0") == 0) {
							printf("변경에 실패했습니다.. 올바른 ID 또는 PW를 입력해주세요.\n");
						}
					}
				}
			}
			//사용자 정보 삭제
			else if (command == 2) {
				printf("삭제할 ID를 입력해주세요: ");
				gets(id);

				printf("정말 삭제하시겠습니까? (Y / N): ");
				scanf("%c", &command);

				if (command == 'Y' || command == 'y') { //Y 또는 y를 입력받으면
					sprintf(sendMsg, "User/Delete/1/%s", id);
					send(sock, sendMsg, strlen(sendMsg), 0); //서버로 사용자 정보 삭제 신호를 send
					printf("사용자 정보를 삭제했습니다.\n");
				}
				else if (command == 'N' || command == 'n') { //N 또는 n을 입력받으면
					sprintf(sendMsg, "User/Delete/0");
					send(sock, sendMsg, strlen(sendMsg), 0); //서버로 사용자 정보 삭제 취소 신호를 send
					printf("사용자 정보 삭제를 취소했습니다.\n");
					continue;
				}
			}
			//사용자 정보 추가
			else if (command == 3) {
				printf("추가할 ID를 입력해주세요: ");
				gets(id); //추가할 ID를 입력받음

				printf("추가할 PW를 입력해주세요: ");
				gets(pw); //추가할 PW를 입력받음

				sprintf(sendMsg, "User/Add/%s/%s", id, pw);
				send(sock, sendMsg, strlen(sendMsg), 0); //서버로 추가할 ID와 PW가 담긴 메시지를 send

				printf("사용자 정보가 추가되었습니다!\n");
			}
		}
		//프로그램 종료
		else if (command == 3) {
			printf("프로그램을 종료합니다.\n");
			send(sock, "EXIT", 4, 0); // 서버로 프로그램 종료 신호를 send
			break; //기능 선택 반복문 종료
		}
		//예외 처리
		else {
			printf("잘못된 입력입니다.\n");
		}
	}

	//소켓 종료 처리
	closesocket(sock);
	WSACleanup();
	
	//프로그램 종료
	return 0;
}

//오류 상황 처리 함수
void ErrorHandling(char* msg) {
	fputs(msg, stderr); //매개변수로 받은 메시지를 출력
	fputc('\n', stderr); //개행 문자 출력
	exit(1); //프로그램 종료
}