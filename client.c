#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFLEN 256
#define LOGIN 1
#define LOGOUT 2
#define LISTSOLD 3
#define TRANSFER 4
#define UNLOCK 5
#define QUIT 6

int g = 0;

struct status {
	char logged;
};

struct status status;
char errcode;
int logfd;
char correctcredentials[200];

void initstatus() {
	status.logged = 0;
}

void error(char *msg) {
	perror(msg);
	exit(0);
}

void errortolog() {
	switch (errcode) {
		case -1:
			write(logfd, "IBANK> -1 : Clientul nu este autentificat\n", 42);
			printf("IBANK> -1 : Clientul nu este autentificat\n");
			break;
		case -2:
			write(logfd, "IBANK> -2 : Sesiune deja deschisa\n", 34);
			printf("IBANK> -2 : Sesiune deja deschisa\n");
			break;
		case -3:
			write(logfd, "IBANK> -3 : Pin gresit\n", 23);
			printf("IBANK> -3 : Pin gresit\n");
			break;
		case -4:
			write(logfd, "IBANK> -4 : Numar card inexistent\n", 34);
			printf("IBANK> -4 : Numar card inexistent\n");
			break;
		case -5:
			write(logfd, "IBANK> -5 : Card blocat\n", 24);
			printf("IBANK> -5 : Card blocat\n");
			break;
		case -6:
			write(logfd, "IBANK> -6 : Operatie esuata\n", 28);
			printf("IBANK> -6 : Operatie esuata\n");
			break;
		case -7:
			write(logfd, "IBANK> -7 : Deblocare esuata\n", 29);
			printf("IBANK> -7 : Deblocare esuata\n");
			break;
		case -8:
			write(logfd, "IBANK> -8 : Fonduri insuficiente\n", 33);
			printf("IBANK> -8 : Fonduri insuficiente\n");
			break;
		case -9:
			write(logfd, "IBANK> -9 : Operatie anulata\n", 29);
			printf("IBANK> -9 : Operatie anulata\n");
			break;
		case -10:
			write(logfd, "IBANK> -10 : Eroare nedefinita\n", 14);
			printf("IBANK> -10 : Eroare nedefinita\n");
			break;
	}
}

char equal(char str1[], char str2[]) {
	if (strlen(str1) != strlen(str2)) {
		return 0;
	}	
	for (int i = 0; i < strlen(str1); i++) {
		if (str1[i] != str2[i]) {
			return 0;
		}
	}
	return 1;
}

long int length(char v[]) {
	int middle = strlen(v)/2;
	if (middle * 2 != strlen(v)) {
		return strlen(v);
	}
	char str1[200], str2[200];
	memset(str1, 0, middle);
	memset(str2, 0, middle);
	memcpy(str1, v, middle);
	memcpy(str2, v + middle, middle);
	if (equal(str1, str2)) {
		return middle;
	}
	return strlen(v);
}

char* no7f(char string[]) {
	if (string[strlen(string) - 1] == 127) {
		string[strlen(string) - 1] = 0;
	}
	return string;
}

int main(int argc, char *argv[]) {
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[BUFLEN];
	char recvbuffer[BUFLEN];
	if (argc < 3) {
  		fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
  		exit(0);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	   	error("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &serv_addr.sin_addr);
	server = gethostbyname(argv[1]);
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);

	if (connect(sockfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
		error("ERROR connecting");   

	char pidno[10];
	sprintf(pidno, "%d", getpid());
	char logfilename[20] = "client-";
	strcat(logfilename, pidno);
	strcat(logfilename, ".log");
	logfd = open(logfilename, O_WRONLY | O_CREAT | O_TRUNC, -1);
	initstatus();
    
	while(1) {
	    	memset(buffer, 0 , BUFLEN);
	    	fgets(buffer, BUFLEN - 1, stdin);


		char islogincommand[6];
		char islogoutcommand[7]; // tot atat ca mai sus!! motivul: login_ si logout (logout fara spatiu la final)
		char islistsoldcommand[9];
		char istransfercommand[9];
		char isquitcommand[5];
		char isunlockcommand[7];
		char c[4];
		char nume[50], prenume[50];
		char* contbuffer;

		memcpy(islogincommand, buffer, 5);
		islogincommand[5] = 0;
		memcpy(islogoutcommand, buffer, 6);
		islogoutcommand[6] = 0;
		memcpy(islistsoldcommand, buffer, 8);
		islistsoldcommand[8] = 0;
		memcpy(istransfercommand, buffer, 8);
		istransfercommand[8] = 0;
		memcpy(isquitcommand, buffer, 4);
		isquitcommand[4] = 0;
		memcpy(isunlockcommand, buffer, 6);
		isunlockcommand[6] = 0;

		if (strcmp(islogincommand, "login") == 0) {	// daca comanda a fost login
			if (status.logged == 1) {	// daca deja eram logat
				errcode = -2;
				errortolog();
			} else {	// daca nu eram logat
				send(sockfd, buffer, strlen(buffer), 0); // trimit comanda (ex. login 111789 8997) la server
				memset(recvbuffer, 0, BUFLEN);
				recv(sockfd, recvbuffer, sizeof(recvbuffer), 0);
				strncpy(c, recvbuffer, strlen(recvbuffer));
				errcode = atoi(c);
				write(logfd, buffer, strlen(buffer));

				if (errcode == -3 || errcode == -4 || errcode == -5) {
					errortolog();
				} else {
					status.logged = 1;
					write(logfd, "IBANK> ", 7);
					write(logfd, "Welcome ", 8);
					write(logfd, recvbuffer, length(recvbuffer));
					write(logfd, "\n", 1);
					printf("IBANK> Welcome %s\n", recvbuffer);
					// pentru celelalte task-uri (folosesc correctcredentials - credentialele corecte ne vor trebui
					memset(correctcredentials, 0, 200);
					memcpy(correctcredentials, buffer, strlen(buffer));
				}
				bzero(recvbuffer, sizeof(recvbuffer));
			}
		} else if (strcmp(islogoutcommand, "logout") == 0) { // daca a fost data comanda logout
			write(logfd, buffer, strlen(buffer));
			if (status.logged == 0) {	// daca nu eram logat
				errcode = -1;
				errortolog();
			} else {	// daca eram logat => trimit la server credentialele pt ca serverul sa stie ce cont trebuie inchis
				buffer[6] = 0;
				memset(buffer, 0, BUFLEN);
				memcpy(buffer, "logout ", 7);
				strcat(buffer, correctcredentials);
				send(sockfd, buffer, strlen(buffer), 0); // s-au trimis numarul de card si pin-ul (CORECTE!)
				write(logfd, "IBANK> ", 7);
				write(logfd, "Clientul a fost deconectat\n", 27);
				printf("IBANK> Clientul a fost deconectat\n");
				status.logged = 0;
			}
		} else if (strcmp(islistsoldcommand, "listsold") == 0) { // daca a fost data comanda listsold
			write(logfd, buffer, strlen(buffer));

			if (status.logged == 0) {
				errcode = -1;
				errortolog();
			} else {
				char strnrcard[7];
				memcpy(strnrcard, correctcredentials + 6, 6);
				strnrcard[6] = 0;
				memset(buffer, 0, BUFLEN);
				memcpy(buffer, "listsold ", 9);
				strcat(buffer, strnrcard);
				buffer[15] = 0;
				send(sockfd, buffer, strlen(buffer), 0); // s-au trimis numarul de card si pin-ul (CORECTE!)
				memset(buffer, 0, BUFLEN);
				recv(sockfd, buffer, 20, 0);
				write(logfd, "IBANK> ", 7);
				char* Sold = strtok(buffer, " ");
				write(logfd, Sold, strlen(Sold));
				write(logfd, "\n", 1);
				printf("IBANK> %s\n", Sold);
			}
		} else if (strcmp(istransfercommand, "transfer") == 0) {
			write(logfd, buffer, strlen(buffer));
			if (status.logged == 0) {
				errcode = -1;
				errortolog();
			} else {
				memmove(buffer + 7, buffer, strlen(buffer));
				memcpy(buffer, "confirm", 7);

				char strnrcard[7];
				memcpy(strnrcard, correctcredentials + 6, 6);
				strnrcard[6] = 0;
				if (buffer[strlen(buffer) - 1] == 10)
					buffer[strlen(buffer) - 1] = 32; // enter se transforma in space
				else {
					strcat(buffer, " ");
				}
				strcat(buffer, strnrcard);	// buffer castiga la final numarcardsrc
				send(sockfd, buffer, strlen(buffer), 0);
				recv(sockfd, buffer, BUFLEN, 0); // PRIMESC PROST BUFFERUL, BRAVO TCPIP!!!!!
				char buffercopy[BUFLEN];
				memset(buffercopy, 0, BUFLEN);
				memcpy(buffercopy, buffer, strlen(buffer));
				if (buffer[0] == '-') { // a primit eroare -4 (card dest inexistent) sau -8 (transfer > suma src)
					if (buffer[1] == '4') {
						errcode = -4;
					} else if (buffer[1] == '8') {
						errcode = -8;
					}
					errortolog();
					continue;
				} else { // transferul se poate efectua (daca dorim)
					char confirmationstr[BUFLEN];
					strtok(buffer, " ");
					strtok(NULL, " ");
					strtok(NULL, " ");
					char* strtransfersold = no7f(strtok(NULL, " "));
					char* numedest = strtok(NULL, " ");
					char* prenumedest = strtok(NULL, " ");
					char numeprenumedest[200];
					memset(numeprenumedest, 0, 200);
					memcpy(numeprenumedest, numedest, strlen(numedest));
					strcat(numeprenumedest, " ");
					strcat(numeprenumedest, prenumedest);
					char tolog[250];
					memset(tolog, 0, 250);
					sprintf(tolog, "IBANK> Transfer %s catre %s? [y/n]\n", strtransfersold, numeprenumedest);//de scris si in log
					write(logfd, tolog, strlen(tolog));
					printf("%s", tolog);
					fgets(confirmationstr, BUFLEN, stdin); // se va lua in considerare prima litera
					if (confirmationstr[strlen(confirmationstr) - 1] == 10) {
						confirmationstr[strlen(confirmationstr) - 1] = 0;
					}
					strcat(confirmationstr, " ");
					write(logfd, confirmationstr, strlen(confirmationstr));
					if (confirmationstr[0] == 'y') { // s-a confirmat ca se va face transferul
						send(sockfd, buffercopy, strlen(buffercopy), 0); // buffercopy contine indecsii care ne intereseaza + strtransfersold + nume dest + prenume dest, in aceasta ordine (dar cel mai la inceput contine "transfer ")
						memset(tolog, 0, 250);
						memcpy(tolog, "\nIBANK> Transfer realizat cu succes\n", 36);
						write(logfd, tolog, strlen(tolog));
						printf("%s", tolog + 1);
					} else {
						errcode = -9;
						errortolog();
					}
				}
			}
		} else if (strcmp (isunlockcommand, "unlock") == 0) {
			
		} else if (strcmp(isquitcommand, "quit") == 0) {
			write(logfd, buffer, strlen(buffer));
			if (status.logged == 0) {
				errcode = -1;
				errortolog();
			} else {
				send(sockfd, isquitcommand, strlen(isquitcommand), 0);
				break;
			}
		}
	}

	close(logfd);
	return 0;
}

