compile:
	g++ -coverage -O0 -pthread message.c client.c -o client
	g++ -coverage -O0 -pthread message.c clientInfo.c server.c -o server
	gcov message.c
	gcov client.c
	gcov clientInfo.c
	gcov server.c
	chmod 710 client
	chmod 710 server

clean:
	rm client
	rm server
