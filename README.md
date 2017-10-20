# LAN-File-Sharing-System

FOR UBUNTU 16.04 :


1. Compile and Execute Server

      g++ -std=c++11 server.cpp -o server

      ./server <port_number>


2. Compile and Execute Client

      g++ -std=c++11 client.cpp -o client
  
      ./client <ip_address of server> <port of server>
