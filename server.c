/*************************************************************************
	> File Name: server.c
	> Author: csgec
	> Mail: 12345678@qq.com 
	> Created Time: 2023年07月10日 星期一 11时12分08秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 4567
#define MAX_BUFFER_SIZE 1024

void handle_upload_request(int client_socket, char* filename) {
    FILE* file;
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_received;
    
    // 打开用于保存上传文件的文件
    file = fopen(filename, "wb");
    if (file == NULL) {
        perror("unable to open file");
        return;
    }
    
    while (1) {
        // 接收文件块的大小
        ssize_t chunk_size;
        if (recv(client_socket, &chunk_size, sizeof(ssize_t), 0) < 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }
        
        // 若接收的大小为0，则表示文件传输完成
        if (chunk_size == 0) {
            break;
        }
        
        // 接收文件块的数据
        ssize_t total_bytes_received = 0;
        while (total_bytes_received < chunk_size) {
            bytes_received = recv(client_socket, buffer, sizeof(buffer)+1, 0);
            if (bytes_received < 0) {
                perror("recv failed");
                exit(EXIT_FAILURE);
            }
            
            // 将接收到的数据写入文件
            fwrite(buffer, 1, bytes_received, file);
            total_bytes_received += bytes_received;
        }
    }
    
    // 关闭文件
    fclose(file);
    
    // 发送上传完成的响应
    const char* response = "Upload completed!";
    if (send(client_socket, response, strlen(response)+1, 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }
}


int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;
    char client_ip[INET_ADDRSTRLEN];
    char buffer[MAX_BUFFER_SIZE];
    
    // 创建套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 设置服务器地址
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);
    
    // 绑定套接字到指定地址和端口
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // 监听连接
    if (listen(server_socket, 1) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", SERVER_PORT);
    
    while (1) {
        // 接受客户端连接
        client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);
        if (client_socket < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        
        // 获取客户端IP地址
        inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Accepted connection from %s\n", client_ip);
        
        // 接收客户端上传请求的文件名
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer)+1, 0);
        if (bytes_received < 0) {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }
        
        // 处理上传请求
        handle_upload_request(client_socket, buffer);
        
        // 关闭客户端套接字
        close(client_socket);
    }
    
    // 关闭服务器套接字
    close(server_socket);
    
    return 0;
}
