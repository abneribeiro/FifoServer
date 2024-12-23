## **FIFO Server**

The **FIFO Server** is a process communication project based on FIFOs (First In, First Out), also known as "named pipes." It implements a client-server system where clients can request file contents from the server. The server processes these requests and sends the requested file content back to the client.

This project demonstrates how FIFOs can be used to implement interprocess communication (IPC), making it ideal for applications requiring simple message exchanges or data transfer between processes in Unix/Linux systems.

---

## **Features**

1. **Server**:
   - Receives client requests containing the name of a file.
   - Processes requests in child processes, allowing multiple clients simultaneously.
   - Reads the content of the requested file and sends it to the corresponding client.
   - Ensures orderly shutdown when receiving the SIGINT signal (`Ctrl+C`), terminating child processes and removing the server FIFO.

2. **Client**:
   - Sends a request to the server specifying the desired file.
   - Receives the file content from the server and displays it in the terminal.
   - Creates an exclusive FIFO for each client based on its PID.

---

## **System Architecture**

The system uses two types of FIFOs for communication:

1. **Server FIFO** (`fifoserver`):
   - A fixed name FIFO used by all clients to send requests to the server.
   - Created by the server during initialization.
   - Opened in read-only mode (`O_RDONLY`) by the server to receive requests.
   - Opened in write-only mode (`O_WRONLY`) by clients to send requests.

2. **Client FIFO**:
   - A dynamically named FIFO based on the client's PID.
   - Each client creates its own FIFO to receive responses from the server.
   - Opened in read-only mode (`O_RDONLY`) by the client to read responses.
   - Opened in write-only mode (`O_WRONLY`) by the server to send responses.

### **Communication Flow**

1. **Client to Server**:
   - The client writes a request to `fifoserver` containing:
     - The client's PID (to identify its exclusive FIFO).
     - The name of the requested file.

2. **Server to Client**:
   - The server reads the request from `fifoserver`.
   - Processes the request and creates a child process to handle it.
   - The child process opens the client's exclusive FIFO and sends the file content.

---

## **Project Execution**

### **Requirements**

- Unix/Linux system.
- GCC compiler.
- Standard C library.

### **Compilation**

Compile the server and client using GCC:
```bash
gcc -o server server.c
gcc -o client client.c
```

### **Execution**

#### **1. Start the Server**
Start the server to wait for client requests:
```bash
./server
```

#### **2. Run the Client**
Run the client, specifying the file you want to request from the server:
```bash
./client <file_name>
```
Example:
```bash
./client example.txt
```

## **Technical Details**

### **Server**

1. **Server FIFO Creation**:
   - The server creates the `fifoserver` FIFO to receive client requests.
   ```c
   unlink("fifoserver");
   mkfifo("fifoserver", 0666);
   ```

2. **Request Handling**:
   - The server opens `fifoserver` in read-only mode (`O_RDONLY`) and waits for client requests.
   - Each request contains:
     - The client's PID.
     - The name of the requested file.

3. **Child Process for Requests**:
   - For each request, the server creates a child process using `fork()`:
     - The child process opens the requested file and the client's exclusive FIFO.
     - Sends the file content to the client's FIFO.

4. **Graceful Shutdown**:
   - The server handles the SIGINT signal (`Ctrl+C`) to shut down cleanly:
     - Sends SIGTERM to all child processes.
     - Removes the `fifoserver` FIFO.

---

### **Client**

1. **Creating an Exclusive FIFO**:
   - Each client creates a FIFO based on its PID:
   ```c
   snprintf(fifo_cli_name, sizeof(fifo_cli_name), "%d", getpid());
   mkfifo(fifo_cli_name, 0666);
   ```

2. **Sending a Request**:
   - The client opens `fifoserver` in write-only mode (`O_WRONLY`) and sends a request containing:
     - The PID.
     - The name of the requested file.

3. **Receiving a Response**:
   - The client opens its exclusive FIFO in read-only mode (`O_RDONLY`) and reads the content sent by the server.

4. **Cleanup**:
   - After receiving the response, the client removes its exclusive FIFO.
