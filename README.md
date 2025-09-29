# TLV Server
## Design & Assumptions Made.
The server is designed as a multi-threaded TCP server that listens on a specified port and handles multiple clients concurrently. Each client connection is accepted on the main listening socket and then processed in a dedicated thread, where the server reads a simple TLV (Type-Length-Value) protocol.The server supports three message types: Hello, Data, and Goodbye.

Logging is centralized through a singleton Logger so all client threads can safely write output. 

Connections are closed 
    1. Either when a Goodbye message is received
    2. The client is idle longer than the configured timeout
    3. When Error/Unknown Messages received.
    4. When the server shuts down. Shutdown is handled by a signal handler that stops accepting new connections, closes the listening socket, and then closes all active client sockets in an orderly fashion.
    
## Build
### Server
#### Cmake
    # go to tlv_server folder
    mkdir build && cd build
    cmake ..
    make

## Run Server
    ./tlv_server PORT 
    ex: ./tlv_server 5222

#### or 
    g++ -std=c++17 -Wall -Iinclude src/main.cpp src/logger.cpp src/server.cpp -o tlv_server
    

# TLV Client(for testing)
## How it works. 
The tlv_client is a TCP client that connects to a server using sockets, formats messages in Type-Length-Value (TLV) format, and can spawn multiple threads to simulate concurrent connections for load or stress testing. It’s mainly used to send TLV requests and validate server handling under parallel traffic.

## Build
#### Client
    # go to tlv_client folder
    mkdir build && cd build
    cmake ..
    make

#### or 
    g++ -std=c++17 -Wall -Isrc src/main.cpp src/client.cpp src/tlv.cpp -o tlv_client
    
## Run Test Client.
 Follow client_run_instructions.txt
 
 Here are the examples from there. 
### Examples
### Short-lived clients

    1 short client
    ./tlv_client 5111 short

    50 parallel short clients
    ./tlv_client 5111 short 50

### Long-lived clients

    1 long client, runs 30s, sending every 5s
    ./tlv_client 5111 long
    
    20 long clients, each 60s, TLV every 10s
    ./tlv_client 5111 long 20 60 10

### Burst clients

    1 burst client, 100 TLVs of 15 bytes (default)
    ./tlv_client 5111 burst
    
    100 burst clients, each sends 1000 TLVs of 64 bytes
    ./tlv_client 5111 burst 100 1000 64

### Mixed clients

    1 mixed client
    ./tlv_client 5111 mixed
    
    20 parallel mixed clients
    ./tlv_client 5111 mixed 20

### Scenario (combination)
    
    Default mix: 10 short, 5 long, 5 buggy short, 5 buggy long
    ./tlv_client 5111 scenario
    
    Custom mix: 50 short, 20 long, 10 buggy short, 10 buggy long
    ./tlv_client 5111 scenario 50 20 10 10




