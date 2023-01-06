#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>

char data[128];

// gönderilen kelimenin uzunluğunu bulunur ve sonuna boşluk konur
int kelimeSonlandirma(char *kelime)
{
    int size = 0;
    int i = 0;
    while (kelime[i] != '\0')
    {
        size++;
        i++;
    }

    if (kelime[size - 1] == '\n')
    {
        kelime[size - 1] = '\0';
    }
}

int main()
{
    int dos;
    int kontrol = 1;
    char *myfifo = "/tmp/file_manager_named_pipe";
    // myfifo oluşturuldu
    mkfifo(myfifo, 0666);
    char response2[128];
    char init[128] = "init";
    dos = open(myfifo, O_WRONLY);
    write(dos, init, sizeof(init));
    close(dos);

    while (kontrol)
    {
        char response[128];
        // kullanıcıdan input alındı
        fgets(data, 128, stdin);
        // inputun sonu \0 ile bitirildi
        kelimeSonlandirma(data);
        dos = open(myfifo, O_WRONLY);
        write(dos, data, sizeof(data));
        close(dos);
        // çıkış yapılır
        if (strcmp(data, "exit") == 0)
        {
            kontrol = 0;
        }
        dos = open(myfifo, O_RDONLY);
        read(dos, response, 128);
        printf("%s\n", response);
    }
    return 0;
}