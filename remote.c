#include "sxiv.h"

#include <stdlib.h>
#include <libgen.h>
#include <curl/curl.h>

char* get_path_from_url(char* url) {
    char* filename = basename(url);

    char* path = malloc(sizeof(char) * 1024);
    sprintf(path, "/tmp/img-%d-%s", rand(), filename);
    return path;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int download_from_url(char* url, char* path) {
    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl = curl_easy_init();
    if(!curl) return 0;

    fp = fopen(path, "wb");
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    fclose(fp);

    if(res != CURLE_OK) return 0;
    return 1;
}
