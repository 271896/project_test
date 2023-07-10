#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 4567
#define MAX_BUFFER_SIZE 1024

using namespace cv;

void handle_upload_request(int server_socket, const char* filename) {
    // 打开要发送的文件
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Unable to open file");
        return;
    }

    // 发送文件上传命令和文件名
    char upload_cmd[MAX_BUFFER_SIZE];
    sprintf(upload_cmd, "upload %s", filename);
    if (send(server_socket, upload_cmd, strlen(upload_cmd), 0) < 0) {
        perror("Send failed");
        return;
    }

    // 发送文件内容
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(server_socket, buffer, bytes_read, 0) < 0) {
            perror("Send failed");
            return;
        }
    }

    // 发送文件传输完成标志
    char completion_flag[] = "complete";
    if (send(server_socket, completion_flag, strlen(completion_flag)+1, 0) < 0) {
        perror("Send failed");
        return;
    }

    // 关闭文件
    fclose(file);

    // 输出上传完成提示信息
    printf("File upload completed.\n");
}

int main() {
    const int FPS = 30;
    const char* OUTPUT_VIDEO_FILENAME = "video_output.avi";

    // 打开默认摄像头
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        printf("Failed to open the camera.\n");
        return -1;
    }

    // 获取摄像头的帧宽和帧高
    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);

    // 创建 VideoWriter 对象来写入视频文件
    VideoWriter writer(OUTPUT_VIDEO_FILENAME, VideoWriter::fourcc('M','J','P','G'), FPS, Size(frame_width, frame_height));
    if (!writer.isOpened()) {
        printf("Failed to open the output video file.\n");
        return -1;
    }

    // 录制视频
    Mat frame;
    char key;
    int recording = 1;
    while (recording) {
        cap >> frame;
        imshow("Video Capture", frame);
        writer.write(frame);

        // 检测键盘输入
        key = waitKey(1);
        if (key == 'q' || key == 'Q') {
            recording = 0; // 按下 'q' 键或 'Q' 键停止录制
        }
    }

    // 关闭窗口
    destroyAllWindows();

    // 释放 VideoWriter
    writer.release();

    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        return -1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Invalid address specified");
        return -1;
    }

    // 连接服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        return -1;
    }

    // 发送视频文件给服务器
    handle_upload_request(sockfd, OUTPUT_VIDEO_FILENAME);

    // 关闭套接字
    close(sockfd);

    // 输出发送完成提示信息
    printf("File sent. Press ENTER to exit.\n");
    getchar();

    return 0;
}
