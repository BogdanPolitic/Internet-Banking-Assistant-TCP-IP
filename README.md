# Internet-Banking-Assistant-TCP-IP

Full explanation about project functionality offered in the "Task.pdf" file.
Full explanation about the algorithm implementation offered in the "README" file.
Short example of program functionality:

Running commands:
--> make

--> ./server 5353 users_data_file &

--> ./client 127.0.0.1 5353

From inside the ./client :

--> login 456127 8795

IBANK> -4 : Numar card inexistent

--> login 456123 87992

IBANK> -3 : Pin gresit

--> login 123456 1234

IBANK> -4 : Numar card inexistent

--> login 456123 8799

IBANK> Welcome Popovici George

--> login 123456 1233

IBANK> -2 : Sesiune deja deschisa

--> quit

In this example, the server is running on the same host machine, thus we set the IP the localhost (127.0.0.1).

The "users_data_file" file present in the current directory is just part of this example. It doesn't affect the program functionality. You can change it however you desire, as long as it respects the users' file structure specified in the "Task.pdf".
