all : 
	g++ -o echo-client echo-client.cpp -l pthread
	g++ -o echo-server echo-server.cpp -l pthread

echo-client : echo-client.cpp
	g++ -o echo-client echo-client.cpp -l pthread

echo-server : echo-server.cpp
	g++ -o echo-server echo-server.cpp -l pthread

clean:
	rm -f echo-client;
	rm -f echo-server;

