all:
	g++ -pthread -O3 -s console.cpp -o console.cgi
	g++ -pthread -o http_server http_server.cpp