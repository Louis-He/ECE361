compile:
	g++ -pthread message.c client.c -o client
	g++ message.c clientInfo.c server.c -o server
	chmod 710 client
	chmod 710 server

clean:
	rm client
	rm server
