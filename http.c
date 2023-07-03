#include <stdio.h>
#include <stdlib.h>
#include <microhttpd.h>

int send_directory_response(struct MHD_Connection *connection, const char *path) {
    char *page;
    int page_length;

    // Đọc nội dung của thư mục
    DIR *dir;
    struct dirent *entry;
    dir = opendir(path);
    if (dir == NULL) {
        return MHD_NO;
    }

    // Tạo HTML cho trang thư mục
    page_length = asprintf(&page, "<html><body><ul>");
    while ((entry = readdir(dir)) != NULL) {
        char *entry_path;
        asprintf(&entry_path, "%s/%s", path, entry->d_name);
        struct stat entry_stat;
        stat(entry_path, &entry_stat);

        if (S_ISDIR(entry_stat.st_mode)) {
            // Thư mục con
            page_length = asprintf(&page, "%s<li><a href=\"%s\">%s/</a></li>", page, entry->d_name, entry->d_name);
        } else {
            // Tập tin
            page_length = asprintf(&page, "%s<li><a href=\"%s\">%s</a></li>", page, entry->d_name, entry->d_name);
        }

        free(entry_path);
    }
    page_length = asprintf(&page, "%s</ul></body></html>", page);

    // Gửi phản hồi cho trình duyệt
    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(page_length, page, MHD_RESPMEM_MUST_FREE);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    closedir(dir);

    return ret;
}

int handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
                   const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
    if (strcmp(method, "GET") == 0) {
        // Lấy đường dẫn tương đối của thư mục gốc
        char *root_path = ".";
        char *full_path;
        asprintf(&full_path, "%s%s", root_path, url);

        struct stat path_stat;
        stat(full_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            // Thư mục
            send_directory_response(connection, full_path);
        } else {
            // Tập tin
            FILE *file = fopen(full_path, "r");
            if (file != NULL) {
                struct MHD_Response *response;
                response = MHD_create_response_from_fd(fileno(file), 0);
                MHD_add_response_header(response, "Content-Type", "text/plain");
                int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                MHD_destroy_response(response);
                fclose(file);
                free(full_path);
                return ret;
            }
        }

        free(full_path);
    }

    // Mặc định trả về lỗi "Không tìm thấy"
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);

    return ret;
}

int main() {
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, 8080, NULL, NULL, &handle_request, NULL,
                              MHD_OPTION_END);
    if (daemon == NULL) {
        printf("Không thể bắt đầu HTTP server.\n");
        return 1;
    }

    printf("HTTP server đang chạy trên cổng 8080...\n");

    getchar();

    MHD_stop_daemon(daemon);

    return 0;
}
