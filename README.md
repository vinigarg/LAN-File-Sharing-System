# LAN-File-Sharing-System

FOR UBUNTU 16.04 :


1. Compile and Execute Server

      g++ -std=c++11 server.cpp -o server

      ./server <server_port>


2. Compile and Execute Client

      g++ -std=c++11 client.cpp -o client
  
      ./client <server_ip_address> <server_port>
