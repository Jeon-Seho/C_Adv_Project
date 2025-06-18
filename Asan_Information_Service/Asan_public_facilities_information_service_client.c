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

	char recvMsg[MAX_BUF], sendMsg[MAX_BUF]; //수신 및 송신 메시지 버퍼
	int msgLen = 0; //수신받은 메시지 버퍼의 크기
	char id[MAX_LEN], pw[MAX_LEN]; //ID와 PW 
	int loginCheck = 0; //로그인 성공 여부
	int permission = 0; //권한

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
	printf("아산시 공공시설 안내 서비스에 오신 것을 환영합니다!\n");
	while (1) {
		char command[10];

		printf("\n1. 회원가입 2. 회원 로그인 3. 비회원 로그인\n입력: ");
		gets(command);

		//회원가입
		if (strcmp(command, "1") == 0) {
			char checkPw[MAX_LEN]; //비밀번호 확인

			printf("ID를 입력해주세요(19자 이내): ");
			gets(id);
			printf("PW를 입력해주세요(19자 이내): ");

			int i = 0;
			while (i < MAX_LEN - 1) {
				pw[i] = _getch();

				if (pw[i] == '\r') { //getch은 엔터 입력을 \r과 \n 두 단계에 걸쳐 처리함
					pw[i] = '\0';
					printf("\n");
					break;
				}
				printf("*"); //getch 함수는 입력이 화면에 출력되지 않으므로 대신 * 출력
				i++;
			}
			
			//비밀번호 재확인
			printf("PW를 한 번 더 입력해주세요: ");
			i = 0;
			while (i < MAX_LEN - 1) {
				checkPw[i] = _getch();

				if (checkPw[i] == '\r') { //getch은 엔터 입력을 \r과 \n 두 단계에 걸쳐 처리함
					checkPw[i] = '\0';
					printf("\n");
					break;
				}
				printf("*"); //getch 함수는 입력이 화면에 출력되지 않으므로 대신 * 출력
				i++;
			}

			//비밀번호가 다르다면
			if (strcmp(pw, checkPw) != 0) {
				printf("\n비밀번호가 잘못 입력되었습니다. 회원가입을 다시 시도해주세요.\n");
				continue;
			}

			//서버로 ID와 PW를 포함한 회원가입 신호 송신
			sprintf(sendMsg, "Login/new/%s/%s", id, pw);
			send(sock, sendMsg, strlen(sendMsg), 0);

			//서버의 응답 메시지를 수신
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0';

			char* ptr = strtok(recvMsg, "/");

			if (strcmp(ptr, "Login") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "new") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "success") == 0) {
						printf("\n회원가입에 성공했습니다! 로그인해주세요.\n");
						continue;
					}
					else if (strcmp(ptr, "fail") == 0) {
						printf("\n회원가입에 실패했습니다.. ID를 수정해주세요.\n");
						continue;
					}
				}
			}
		}
		//회원 로그인
		else if (strcmp(command, "2") == 0) {
			
			printf("ID를 입력해주세요: ");
			gets(id);
			printf("PW를 입력해주세요: ");

			int i = 0;
			while (i < MAX_LEN - 1) {
				pw[i] = _getch();

				if (pw[i] == '\r') { //getch은 엔터 입력을 \r과 \n 두 단계에 걸쳐 처리함
					pw[i] = '\0';
					printf("\n");
					break;
				}
				printf("*"); //getch 함수는 입력이 화면에 출력되지 않으므로 대신 * 출력
				i++;
			}

			//서버로 ID와 PW를 포함한 로그인 신호 송신
			sprintf(sendMsg, "Login/member/%s/%s", id, pw);
			send(sock, sendMsg, strlen(sendMsg), 0);

			//서버의 응답 메시지를 수신
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입
			
			char* ptr = strtok(recvMsg, "/");

			//수신받은 메시지가 로그인 신호면
			if (strcmp(ptr, "Login") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "member") == 0) {
					ptr = strtok(NULL, "/");

					//로그인 성공 시
					if (strcmp(ptr, "success") == 0) {
						printf("\n로그인 성공!\n");
						loginCheck = 1;

						//권한 저장
						ptr = strtok(NULL, "/");
						permission = atoi(ptr);
					}
					//로그인 실패 시
					else if (strcmp(ptr, "fail") == 0) {
						printf("\n로그인 실패.. 다시 로그인해주세요.\n");
					}
				}
			}
			
			//로그인 성공 시 반복문 종료
			if (loginCheck) {
				break;
			}
		}
		//비회원 로그인
		else if (strcmp(command, "3") == 0) {
			printf("\n비회원 로그인을 시도합니다...\n");

			//서버로 비회원 로그인 신호 송신
			sprintf(sendMsg, "Login/guest");
			send(sock, sendMsg, strlen(sendMsg), 0);

			//서버의 응답 메시지를 수신
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0'; //recv한 메시지 버퍼의 마지막에 NULL문자 대입

			char* ptr = strtok(recvMsg, "/");

			//수신받은 메시지가 로그인 신호면
			if (strcmp(ptr, "Login") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "guest") == 0) {
					ptr = strtok(NULL, "/");
					
					if (strcmp(ptr, "success") == 0) {
						printf("\n비회원 로그인에 성공했습니다!\n");

						strcpy(id, "guest");
						strcpy(pw, "guest");

						break;
					}
				}
			}

			printf("\n비회원 로그인에 실패했습니다.\n");
		}
		//예외 처리
		else {
			printf("\n잘못된 입력입니다.\n");
			continue;
		}
	}

	printf("\n------------------------------------\n");
	printf("-----아산시 공공시설 안내 서비스----\n");
	printf("------------------------------------\n");

	//기능 선택
	while (1) {
		char command[10];
		int division = -1;
		char tmpMsg[MAX_STR];
		
		printf("\n1. 공공시설 검색 2. 사용자 정보 관리 3. 프로그램 종료\n입력: ");
		gets(command);

		//공공시설 검색
		if (strcmp(command, "1") == 0) {
			printf("\n구분 선택: 1. 장애인시설 2. 사회복지단체 3. 노인복지시설 4. 주차장 5. 쉼터 6. 마을회관\n입력: ");
			scanf(" %d", &division);
			getchar();

			//서버로 선택한 공공시설 구분 송신
			sprintf(sendMsg, "Info/division/%d", division);
			send(sock, sendMsg, strlen(sendMsg), 0);

			//기능 선택
			while (1) {
				//서버의 시설명 목록 응답 메시지 수신
				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
				recvMsg[msgLen] = '\0';

				//시설명 목록 출력
				char* ptr = strtok(recvMsg, "/");
				printf("------------------------------------------------------------------------------------------------------------------------\n");
				printf("%s\n", ptr);
				printf("------------------------------------------------------------------------------------------------------------------------\n");

				//처음 또는 마지막 페이지 신호 확인
				ptr = strtok(NULL, "/");
				if (strcmp(ptr, "start") == 0) {
					printf("<첫 번째 페이지입니다.>\n");
				}
				else if (strcmp(ptr, "end") == 0) {
					printf("<마지막 페이지입니다.>\n");
				}

				printf("\n1. 시설 선택 2. 주소 검색 3. 이전 페이지 4. 이후 페이지\n입력: ");
				gets(command);

				//시설명 입력
				if (strcmp(command, "1") == 0) {
					printf("시설명 입력: ");
					gets(tmpMsg);

					//서버로 선택한 시설명 송신
					sprintf(sendMsg, "Info/select/%s", tmpMsg);
					send(sock, sendMsg, strlen(sendMsg), 0);

					break;
				}
				//주소 검색
				else if (strcmp(command, "2") == 0) {
					printf("주소 입력: ");
					gets(tmpMsg);

					//서버로 검색하고자 하는 주소 송신
					sprintf(sendMsg, "Info/search/%s", tmpMsg);
					send(sock, sendMsg, strlen(sendMsg), 0);

					//서버의 응답 메시지 수신
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0';

					//검색에 실패한 경우
					if (strcmp(recvMsg, "Info/search/fail") == 0) {
						printf("존재하지 않는 주소입니다. 다시 입력해주세요.\n");
						continue;
					}
					//검색에 성공한 경우
					else {
						printf("%s\n", recvMsg); //시설명 목록 출력

						printf("시설명 입력: ");
						gets(tmpMsg);

						//서버로 선택한 시설명 송신
						sprintf(sendMsg, "Info/select/%s", tmpMsg);
						send(sock, sendMsg, strlen(sendMsg), 0);

						break;
					}
				}
				//이전 페이지
				else if (strcmp(command, "3") == 0) {
					//서버로 이전 페이지 신호 송신
					sprintf(sendMsg, "Info/prev");
					send(sock, sendMsg, strlen(sendMsg), 0);
				}
				//다음 페이지
				else if (strcmp(command, "4") == 0) {
					//서버로 다음 페이지 신호 송신
					sprintf(sendMsg, "Info/next");
					send(sock, sendMsg, strlen(sendMsg), 0);
				}
				//예외 처리
				else {
					printf("잘못된 입력입니다.\n");
					continue;
				}
			}
			
			//서버의 선택 시설 상세 정보 수신
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0';

			//시설명 존재 여부 확인
			char* ptr = strtok(recvMsg, "/");

			if (strcmp(ptr, "Info") == 0) {
				ptr = strtok(NULL, "/");

				if (strcmp(ptr, "select") == 0) {
					ptr = strtok(NULL, "/");

					//존재하지 않는 시설명인 경우
					if (strcmp(ptr, "fail") == 0) {
						printf("존재하지 않는 시설명입니다. 다시 입력해주세요.\n");
						continue;
					}
					//존재하는 시설명인 경우
					else {
						printf("\n%s\n", ptr); //시설 상세 정보 출력
					}
				}
			}

			//서버의 시설 후기 정보를 수신
			msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
			recvMsg[msgLen] = '\0';

			//시설 후기 정보 출력
			printf("\n----- Review -----\n");
			printf("%s", recvMsg);

			printf("\n1. 후기 작성 2. 시설 정보 수정 3. 종료\n입력: ");
			gets(command);

			//후기 작성
			if (strcmp(command, "1") == 0) {
				//로그인 여부 확인
				if (loginCheck) {
					printf("후기 입력: ");
					gets(tmpMsg);

					//서버로 ID와 작성한 후기 송신
					sprintf(sendMsg, "Info/review/%s/%s", id, tmpMsg);
					send(sock, sendMsg, strlen(sendMsg), 0);

					//서버의 응답 메시지 수신
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0';

					ptr = strtok(recvMsg, "/");

					if (strcmp(ptr, "Info") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "review") == 0) {
							ptr = strtok(NULL, "/");

							//후기 작성에 성공한 경우
							if (strcmp(ptr, "success") == 0) {
								printf("후기 작성에 성공했습니다!\n");
								continue;
							}
						}
					}

					printf("후기 작성에 실패했습니다..\n");
					continue;
				}
				//비회원인 경우
				else {
					printf("회원가입 또는 로그인해주세요.\n");
					continue;
				}
			}
			//시설 정보 수정
			else if (strcmp(command, "2") == 0) {
				//권한 검사
				if (permission == 1) {
					printf("\n1. 시설명 편집 2. 시설 주소 편집\n입력: ");
					gets(command);

					//시설명 편집
					if (strcmp(command, "1") == 0) {
						printf("새로운 시설명을 입력해주세요: ");
						gets(tmpMsg);

						//서버로 편집하고자하는 시설명 송신
						sprintf(sendMsg, "Info/edit/name/%s", tmpMsg);
						send(sock, sendMsg, strlen(sendMsg), 0);
					}
					//시설 주소 편집
					else if (strcmp(command, "2") == 0) {
						printf("새로운 주소를 입력해주세요: ");
						gets(tmpMsg);

						//서버로 편집하고자하는 주소 송신
						sprintf(sendMsg, "Info/edit/address/%s", tmpMsg);
						send(sock, sendMsg, strlen(sendMsg), 0);
					}
					//예외 처리
					else {
						printf("잘못된 입력입니다.\n");
						continue;
					}

					//서버의 응답 메시지 수신 
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0';

					ptr = strtok(recvMsg, "/");

					if (strcmp(ptr, "Info") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "edit") == 0) {
							ptr = strtok(NULL, "/");

							//정보 수정에 성공한 경우
							if (strcmp(ptr, "success") == 0) {
								printf("정보 수정에 성공했습니다!\n");
								continue;
							}
						}
					}

					printf("정보 수정에 실패했습니다..\n");
					continue;
				}
				else {
					printf("시설 정보를 편집할 권한이 없습니다.\n");
					continue;
				}
			}
			//종료
			else if (strcmp(command, "3") == 0) {
				printf("처음으로 돌아갑니다.\n");
				sprintf(sendMsg, "Info/end");
				send(sock, sendMsg, strlen(sendMsg), 0);

				continue;
			}
			//예외 처리
			else {
				printf("잘못된 입력입니다.\n");
				continue;
			}
		}
		//사용자 정보 관리
		else if (strcmp(command, "2") == 0) {
			//로그인 여부 확인
			if (loginCheck) {
				//비밀번호 확인
				printf("현재 로그인 중인 계정의 PW를 입력해주세요: ");
				gets(pw);

				//서버로 ID와 PW를 송신
				sprintf(sendMsg, "User/checkpw/%s/%s", id, pw);
				send(sock, sendMsg, strlen(sendMsg), 0);

				//서버의 응답 메시지를 수신
				msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
				recvMsg[msgLen] = '\0';

				//비밀번호 일치 여부 확인
				char* ptr = strtok(recvMsg, "/");

				if (strcmp(ptr, "User") == 0) {
					ptr = strtok(NULL, "/");

					if (strcmp(ptr, "checkpw") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "success") == 0) {
							printf("비밀번호가 일치합니다. 다음 메뉴로 이동합니다.\n");
						}
						else if (strcmp(ptr, "fail") == 0) {
							printf("비밀번호를 다시 확인해주세요!\n");
							continue;
						}
					}
				}

				printf("\n1. 사용자 정보 수정 2. 사용자 정보 삭제\n입력: ");
				gets(command);

				//사용자 정보 수정
				if (strcmp(command, "1") == 0) {
					char newPw[MAX_LEN];
					printf("새로운 비밀번호를 입력해주세요: ");
					gets(newPw);

					//서버로 ID와 새로운 PW를 송신
					sprintf(sendMsg, "User/edit/%s", newPw);
					send(sock, sendMsg, strlen(sendMsg), 0);

					//서버의 응답 메시지를 수신
					msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
					recvMsg[msgLen] = '\0'; 

					ptr = strtok(recvMsg, "/");

					if (strcmp(ptr, "User") == 0) {
						ptr = strtok(NULL, "/");

						if (strcmp(ptr, "edit") == 0) {
							ptr = strtok(NULL, "/");

							//사용자 정보 수정에 성공한 경우
							if (strcmp(ptr, "success") == 0) {
								printf("비밀번호 변경에 성공했습니다!\n");
							}
							//사용자 정보 수정에 실패한 경우
							else if (strcmp(ptr, "fail") == 0) {
								printf("비밀번호 변경에 실패했습니다..\n");
							}
						}
					}
				}
				//사용자 정보 삭제
				else if (strcmp(command, "2") == 0) {
					printf("정말로 사용자 정보를 삭제하시겠습니까? (Y / N): ");
					gets(command);

					//Y 또는 y를 입력한 경우
					if (strcmp(command, "Y") == 0 || strcmp(command, "y") == 0) {
						//서버로 삭제할 ID를 송신
						sprintf(sendMsg, "User/delete/%s", id);
						send(sock, sendMsg, strlen(sendMsg), 0);

						//서버의 응답 메시지를 수신
						msgLen = recv(sock, recvMsg, sizeof(recvMsg), 0);
						recvMsg[msgLen] = '\0';

						ptr = strtok(recvMsg, "/");

						if (strcmp(ptr, "User") == 0) {
							ptr = strtok(NULL, "/");

							if (strcmp(ptr, "delete") == 0) {
								ptr = strtok(NULL, "/");

								//사용자 정보 삭제에 성공한 경우
								if (strcmp(ptr, "success") == 0) {
									printf("사용자 정보 삭제에 성공했습니다!\n");
									break;
								}
								//사용자 정보 삭제에 실패한 경우
								else if (strcmp(ptr, "fail") == 0) {
									printf("사용자 정보 삭제에 실패했습니다..\n");
									continue;
								}
							}
						}
					}
					//N 또는 n을 입력한 경우
					else if (strcmp(command, "N") == 0 || strcmp(command, "n") == 0) {
						printf("사용자 정보 삭제를 취소합니다.\n");
						continue;
					}
					//예외 처리
					else {
						printf("잘못된 입력입니다.\n");
						continue;
					}
				}
				//예외 처리
				else {
					printf("잘못된 입력입니다.\n");
					continue;
				}
			}
			//비회원인 경우
			else {
				printf("회원가입 또는 로그인해주세요.\n");
				continue;
			}
		}
		//프로그램 종료
		else if (strcmp(command, "3") == 0) {
			printf("프로그램을 종료합니다.\n");
			send(sock, "Exit", 4, 0); // 서버로 프로그램 종료 신호를 send
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