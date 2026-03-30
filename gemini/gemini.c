#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct Buffer {
    char *data;
    size_t size;
};

size_t write_cb(void *ptr,size_t size,size_t nmemb,void *userdata){
    size_t total=size*nmemb;
    struct Buffer *buf=userdata;

    char *tmp=realloc(buf->data,buf->size+total+1);
    if(!tmp) return 0;

    buf->data=tmp;
    memcpy(buf->data+buf->size,ptr,total);
    buf->size+=total;
    buf->data[buf->size]=0;

    return total;
}

// Extract Gemini reply text safely
void print_text(char *json){
    char *p=strstr(json,"\"text\"");
    if(!p){
        printf("No text found\n");
        return;
    }

    p=strchr(p,':');
    if(!p) return;
    p++;

    while(*p && *p!='\"') p++;
    if(!p) return;
    p++;

    while(*p && *p!='\"'){
        if(*p=='\\'){
            p++;
            if(*p=='n') putchar('\n');
            else if(*p=='\"') putchar('\"');
            else if(*p=='t') putchar('\t');
            else putchar(*p);
        } else {
            putchar(*p);
        }
        p++;
    }
    putchar('\n');
}

int main(int argc,char **argv){

    const char *key=getenv("GEMINI_API_KEY");
    if(!key){
        printf("ERROR: export GEMINI_API_KEY first\n");
        return 1;
    }

    const char *prompt=(argc>1)?argv[1]:"Hello";

    char json[2048];
    snprintf(json,sizeof(json),
        "{"
        "\"contents\":[{"
        "\"parts\":[{\"text\":\"%s\"}]"
        "}]"
        "}",prompt);

    char url[512];
    snprintf(url,sizeof(url),
        "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%s",
        key);

    CURL *curl=curl_easy_init();
    if(!curl){
        printf("curl init failed\n");
        return 1;
    }

    struct Buffer buf={0};

    struct curl_slist *headers=NULL;
    headers=curl_slist_append(headers,"Content-Type: application/json");

    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,json);
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_cb);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&buf);

    // disable SSL verify (for minimal Linux)
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,0L);

    CURLcode res=curl_easy_perform(curl);

    if(res!=CURLE_OK){
        printf("Request failed: %s\n",curl_easy_strerror(res));
    }
    else if(buf.data){
        print_text(buf.data);
    }

    free(buf.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return 0;
}