#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <algorithm>

bool broadcast = false;
bool echo = false;
//소켓 디스크립터 관리를 위한 백터. 붙은 클라이언트들의 디스크립터가 담김
std::vector<int> client_list;;

void usage() {
	printf("syntax : echo-server -p [port] [-e[-b]]\n");
	printf(" -e : echo, -b : broadcast\n");
	printf("sample: echo-server -p 1234 -e -b\n");
}

void recvThread(int sd) {
	printf("connected\n");
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE] = {0, };
	while (true) {
		ssize_t res = ::recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			fprintf(stderr, "recv return %ld", res);
			perror(" ");
			break;
		}
		buf[res] = '\0';
		printf("%s", buf);
		fflush(stdout);
        //echo와 broadcast가 동시에 설정될 경우, 전송한 클라이언트는 2번 수신하기에, broadcast를 먼저수행하고, echo는 수행하지 않음.
        if (broadcast) {
            std::vector<int>::iterator it;
            for(it = client_list.begin(); it != client_list.end(); it++){
                res = ::send(*it, buf, res, 0);
                if (res == 0 || res == -1) {
                    fprintf(stderr, "send return %ld", res);
                    perror(" ");
                    break;
                }
            }
            continue;
		}
		if (echo) {
			res = ::send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				fprintf(stderr, "send return %ld", res);
				perror(" ");
				break;
			}
		}

	}
	printf("disconnected %d\n", sd);
	//특정 클라이언트의 연결이 중단될 경우, 해당 디스크립터를 삭제
    auto it = find(client_list.begin(), client_list.end(), sd);
    client_list.erase(it);
	printf("online : ");
	for(it = client_list.begin(); it != client_list.end(); it++){
		printf("%d, ", *it);
	}
	printf("\n");
	::close(sd);
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		usage();
		return -1;
	}
    uint16_t port{0};
    //Option 처리
    char option;
    while ( -1 != (option = getopt(argc, argv, "p:eb")) ) {
        switch (option) {
             case 'p' :
	            port = atoi(argv[2]);
                break;
            case 'e' :
                echo = true;
                break;
            case 'b' : 
                broadcast = true;
                break;
            default:
                printf("Invaild Option\n");
                return 1;
        };
    };

    printf("result : echo %d, broadcast %d\n", echo, broadcast);

	int sd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int res;
	int optval = 1;
	res = ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5);//최대 5개의 연결 허용
	if (res == -1) {
		perror("listen");
		return -1;
	}
    printf("Server Ready\n");

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = ::accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		printf("client %d join the server\n", cli_sd);
        client_list.push_back(cli_sd);
		std::thread* t = new std::thread(recvThread, cli_sd);
		t->detach();
	}
	::close(sd);
}
