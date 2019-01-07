# C-Fork-Based-Server

This system has 3 components:
1.Network element: a simple TCP/IP client that periodically measures the time of some computation and sends the result to the server.
2.Control Server: A dummy server that “talks” to the connected clients using TCP/IP.
  The server keeps information regarding the overall and average computation overhead of the connected clients.
3.Monitor Service: A management console that communicates with the server using UNIX domain sockets.
  The management console allows sending simple queries to the server (whois/getworst).
  
Basic Usage
1.Run Fork-Based-Server 
2.Run one or more TCP-IP-Client 
3.Run UDS-Client 
4.Type whois/getworst/grep commands and see the results from server
  
