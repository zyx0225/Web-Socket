Full Name: Yixin Zhang
Student ID: 3359542992
I have finished phase1, phase2, phase3 of the final project.
1. What the code files are.
directory_server: This is the code for the directory server in both phase1 and phase2. In phase1, the directory server create a UDP socket and receive registration info from file servers. In phase2, the directory server create a UDP socket receive and response to request from client.
file_server1: This is the code for the file server1 in both phase1 and phase3. In phase1, the file server1 create a UDP socket and send registration info to directory server. In phase2, the file server1 create a TCP socket to receive and response to request from client.
file_server2: This is the code for the file server2 in both phase1 and phase3. In phase1, the file server2 create a UDP socket and send registration info to directory server. In phase2, the file server2 create a TCP socket to receive and response to request from client.
file_server3: This is the code for the file server3 in both phase1 and phase3. In phase1, the file server3 create a UDP socket and send registration info to directory server. In phase2, the file server3 create a TCP socket to receive and response to request from client.
client1: This is the code for the client1 in both phase2 and phase3. In phase1, the client1 create a UDP socket and send and recvfrom the directory server. In phase3, client1 create a TCP socket send and recv from the file server.
client2: This is the code for the client2 in both phase2 and phase3. In phase1, the client2 create a UDP socket and send and recvfrom the directory server. In phase3, client2 create a TCP socket send and recv from the file server.
2. How to run the progrom.
(1) Complie all these .c files with command "gcc -o yourfileoutput yourfile.c -lnsl -lresolv".
(2) Run the directory_server. (waiting to recv registration info from file server)
(3) Run the file_server1. (sending registration info to directory server and then waiting to recv request from client)
(4) Run the file_server2. (same as before)
(5) Run the file_server3. (same as before)
(6) Run the client1. (sending request to direcotry server, recvfrom directory_server, sending request to file server, recv file from file server)
(7) Run the client2. (same as before)
(8) You can change the topology.txt and resource.txt, so the client would get 
3. Some claims
(1) When complie these files, there would a "warning: no newline at end of file". But this won't affect the program runnning.
(2) All the file operations (fopen, fclose, access, remove) to directory.txt, resource.txt, topology.txt, uses the file path "/home/scf-12/yixinz/name.txt". You may have to change them when run program on different machine.
(3) There is a difference between the line break character in Unix (LF) and Windows (CR/LF). Because these files are used in Unix, so I used the LF in all the txt files. In order to view and change these files in right format, you should use a software which can recognize the Unix line break symbol. Otherwise, the program may fail because of the file operations.
