#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS	5
#define BUFLEN 256
#define LOGIN 1
#define LOGOUT 2
#define LISTSOLD 3
#define TRANSFER 4
#define UNLOCK 5
#define QUIT 6
#define CONFIRMTRANSFER 7

struct data {
	char nume[50];
	char prenume[50];
	int numar_card;
	short pin;
	char parola_secreta[8];
	double sold;
	char logged; // T 1, F 0
	char blocked; // retine nr_attempts. =3 inseamna blocat
};

struct database {
	struct data* records;
	int len;
};

struct database database;

char isloggedin(int cardno) { // presupun ca exista in database
	int nr = database.records[0].numar_card;
	int i = 0;
	while (nr != cardno) {
		nr = database.records[++i].numar_card;
	}
	return database.records[i].logged;
}

char isblocked(int cardno) { // presupun ca exista in database
	char nr = database.records[0].numar_card;
	int i = 0;
	while (nr != cardno) {
		nr = database.records[++i].numar_card;
	}
	return database.records[i].blocked > 3;
}

void writeinstruct(char usersdatafile[]) {
	FILE* f = fopen(usersdatafile, "r");
	char str[200];
	int nr;

	fgets(str, 10, f);
	strncpy(str, str, strchr(str, 10) - str);
	nr = atoi(str);
	fseek(f, strlen(str), SEEK_SET);
	char matr[nr][200];
	for (int i = 0; i < nr; i++) {
		memset(matr[i], 0, 200);
		fgets(matr[i], 200, f);
	}
	const char delim[2] = " ";
	char* token = (char*)malloc(50);
	database.records = (struct data*)malloc(nr * sizeof(struct data));
	database.len = nr;

	for (int i = 0; i < nr; i++) {
		token = strtok(matr[i], delim);
		memcpy(database.records[i].nume, token, strlen(token));
		token = strtok(NULL, delim);
		memcpy(database.records[i].prenume, token, strlen(token));
		token = strtok(NULL, delim);
		database.records[i].numar_card = atoi(token);
		token = strtok(NULL, delim);
		database.records[i].pin = (short) atoi(token);
		token = strtok(NULL, delim);
		memcpy(database.records[i].parola_secreta, token, strlen(token));
		token = strtok(NULL, delim);
		database.records[i].sold = atof(token);
		database.records[i].logged = 0;
		database.records[i].blocked = 1;
	}

	fclose(f);
}

void commandname(char str[], char alsonoenter) {
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == 10 || str[i] == 13 || (alsonoenter && str[i] == 32)) {
			str[i] = 0;
		}
	}
}

char whichcommand(char command[]) {
	char commandcopy[200];
	memset(commandcopy, 0, 200);
	memcpy(commandcopy, command, strlen(command));
	commandname(commandcopy, 1);
	if (strcmp("login", commandcopy) == 0)
		return (char) LOGIN;
	if (strcmp("logout", commandcopy) == 0)
		return (char) LOGOUT;
	if (strcmp("listsold", commandcopy) == 0)
		return (char) LISTSOLD;
	if (strcmp("confirmtransfer", commandcopy) == 0)
		return (char) CONFIRMTRANSFER;
	if (strcmp("transfer", commandcopy) == 0)
		return (char) TRANSFER;
	if (strcmp("unlock", commandcopy) == 0)
		return (char) UNLOCK;
	if (strcmp("quit", commandcopy) == 0)
		return (char) QUIT;
	return 0;
}

void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, portno, clilen;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i = 0, j = 0, k = 0;

	writeinstruct(argv[2]);

	fd_set read_fds;	//multimea de citire folosita in select()
	fd_set tmp_fds;	//multime folosita temporar 
	int fdmax;		//valoare maxima file descriptor din multimea read_fds

	if (argc < 2) {
		fprintf(stderr, "Usage : %s port\n", argv[0]);
		exit(1);
	}

	//golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	int yes = 1;
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		error("setsockopt error");
		exit(1);
	}

	portno = atoi(argv[1]);

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
	serv_addr.sin_port = htons(portno);
     
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
		error("ERROR on binding");
     
	listen(sockfd, MAX_CLIENTS);

	//adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	// main loop
	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
			
				if (i == sockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) => o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}
					
				else {
					// am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("selectserver: socket %d hung up\n", i);
						} else {
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
					}
					
					else { //recv intoarce >0
						switch (whichcommand(buffer)) {
							case LOGIN: {
								char delim[2] = " ";
								int numar_card;
								short pin;
								char errorgiven = 0;
								strtok(buffer, delim);
								numar_card = atoi(strtok(NULL, delim));
								pin = (short) atoi(strtok(NULL, delim));
								for (j = 0; database.records[j].numar_card != numar_card; j++) {
									if (j == database.len) {
										// -4
										memcpy(buffer, "-4", 2);
										send(i, buffer, strlen(buffer), 0);
										errorgiven = 1;
										break;
									}
								}
								if (!errorgiven) {
									if (database.records[j].blocked == 3) {
											memcpy(buffer, "-5", 2);
											send(i, buffer, strlen(buffer), 0);
									} else if (pin != database.records[j].pin) {
											memcpy(buffer, "-3", 2);
											send(i, buffer, strlen(buffer), 0);
											bzero(buffer, sizeof(buffer));
											database.records[j].blocked++;
											errorgiven = 1;
									} else {
										database.records[j].blocked = 1;
										memset(buffer, 0, BUFLEN);
										strcat(buffer, database.records[j].nume);
										strcat(buffer, " ");
										strcat(buffer, database.records[j].prenume);
										send(i, buffer, strlen(buffer), 0);
										database.records[j].logged = 1;
									}
								}
								memset(buffer, 0, BUFLEN);
								break; // PERICOL DE SEGFAULT FARA BREAK!!
							}
							case LOGOUT: {
								char strnrcard[7];
								int numar_card;
								memcpy(strnrcard, buffer + 7, 6);//argv2=6
								strnrcard[6] = 0;
								numar_card = atoi(strnrcard);
								for (j = 0; database.records[j].numar_card != numar_card; j++) {
								}
								database.records[j].logged = 0;
								break;
							}
							case LISTSOLD: {
								char strsold[20];
								char strnrcard[7];
								int numar_card;
								memcpy(strnrcard, buffer + 9, 6);
								strnrcard[6] = 0;
								numar_card = atoi(strnrcard);
								for (j = 0; database.records[j].numar_card != numar_card; j++) {
								}
								memset(strsold, 0, 20);
								sprintf(strsold, "%.2lf", database.records[j].sold);
printf("strsold=%s.\n", strsold);
								send(i, strsold, strlen(strsold), 0);
								break;
							}
							case CONFIRMTRANSFER: {
								char strnrcard[7];
								char strtransfersold[20];
								char strmysold[7];
								int numarcardsrc;
								int numarcarddest;
								double mysold;
								double transfersold;
								char errortaken = 0;
								memcpy(strnrcard, buffer + strlen("confirmtransfer "), 6);
								strnrcard[6] = 0;
								numarcarddest = atoi(strnrcard);
								for (j = 0; database.records[j].numar_card != numarcarddest; j++) {
									if (j == database.len) { // adica nu s-a gasit destinatarul
										memset(buffer, 0, BUFLEN);
										sprintf(buffer, "%d", -4);
										errortaken = 1;
										break;
									}
								}
								if (errortaken) {
									send(i, buffer, 2, 0); // -4
									break;
								} else {
									memcpy(strnrcard, buffer + strlen(buffer) - 6, 6);
									strnrcard[6] = 0;
									numarcardsrc = atoi(strnrcard);
									for (k = 0; database.records[k].numar_card != numarcardsrc; k++) {
									}
									mysold = database.records[k].sold;
									memset(strtransfersold, 0, 20);
									memcpy(strtransfersold, buffer + 23, strlen(buffer) - 30);//notSure
									transfersold = atof(strtransfersold);
									memset(buffer, 0, BUFLEN);
									if (transfersold > mysold) {
										sprintf(buffer, "%d", -8);
										errortaken = 1;
										send(i, buffer, 2, 0); // -8
										break;
									} // if errortaken == 0 : v - se cere confirmarea:
									memset(buffer, 0, BUFLEN);
									char values[10];
									memset(values, 0, 10);
									sprintf(buffer, "transfer "); // ne va ajuta la intrarea din nou in switch (daca transferul este confirmat)
									sprintf(buffer + strlen(buffer), "%d ", k);
									sprintf(buffer + strlen(buffer), "%d ", j);
									sprintf(buffer + strlen(buffer), "%s ", strtransfersold);
									sprintf(buffer + strlen(buffer), "%s ", database.records[j].nume);
									sprintf(buffer + strlen(buffer), "%s", database.records[j].prenume);
									// UNDE: k = indexul sursei
									//	j = indexul destinatiei
									//(cu atatea sprintf-uri ne va fi oricum usor sa folosim strtok)
									send(i, buffer, BUFLEN, 0); //nu incepe cu - =>noerror
								} break;
							}
							case TRANSFER: {
printf("%s\n", buffer);
								strtok(buffer, " ");
								int indexsrc = atoi(strtok(NULL, " "));
								int indexdst = atoi(strtok(NULL, " "));
								int transfersold = atof(strtok(NULL, " "));
								database.records[indexsrc].sold -= transfersold;
								database.records[indexdst].sold += transfersold;
								break;
							}
							case QUIT: {
								close(i); 
								FD_CLR(i, &read_fds); 
							}
							default: break;
						}
					}
				}
			}
		}
	}


	close(sockfd);
   
	return 0; 
}
