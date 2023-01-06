#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>

// değişkenker, lock, cond ve struct tanımlandı
char dosya_listesi[10][10];
char response[128];
char c;
int client_size = 0;
pthread_t threads[5];
pthread_mutex_t lock;
pthread_cond_t cond;
// argumanlar struct ile tutuldu
struct parameters
{
    char *arg1;
    char *arg2;
};

// girilen argümanları boşluğa göre bölüştürme metodu
char **komutBolustur(char *str)
{
    char *token = strtok(str, " ");
    char **commands = malloc(10 * sizeof(char *));
    for (int i = 0; i < 10; i++)
    {
        commands[i] = malloc(10 * sizeof(char));
    }

    int index = 0;
    while (token != NULL)
    {
        *(commands + index) = token;
        index++;
        token = strtok(NULL, " ");
    }

    return commands;
}

int dosyaKontrol(char *dosya_adi)
{
    // dosyanın olup olmama kontrolü
    int kontrol = -1;
    for (int i = 0; i < 10; i++)
    {
        if (dosya_listesi[i] != NULL && strcmp(dosya_listesi[i], dosya_adi) == 0)
        {
            kontrol = i; // dosya bulunur
            break;
        }
    }
    return kontrol;
}

void *dosyaOlustur(char *args)
{
    // thread lock
    pthread_mutex_lock(&lock);
    struct parameters *parameters = args;
    // dosyanın adı argümandan bulunur
    char *dosya_adi = parameters->arg1;
    printf("Dosya Adi : %s\n", dosya_adi);

    // kontrol = -1 ise dosya yoktur ve oluşturulur
    if (dosyaKontrol(dosya_adi) == -1)
    {
        for (int i = 0; i < 10; i++)
        {
            if (dosya_listesi[i][0] == NULL)
            {
                // file ismi boş indexe yazılır ve oluşturulur
                strcpy(dosya_listesi[i], dosya_adi);
                FILE *file = fopen(dosya_adi, "w");
                fclose(file);
                strcpy(response, "Dosya olusturuldu!\n");
                break;
            }
        }
    }
    else
    {
        strcpy(response, "Dosya zaten olusturulmus!\n");
        printf("---Dosya Listesi---\n");//Kullanıcının farklı bir dosya ismi seçebilmesi için oluşturulmuş olan dosyalar yazdırıldı
        for (int i = 0; i < 10; i++)
        {
            if (strlen(dosya_listesi[i]) > 0)
            {
                printf("%s\n", dosya_listesi[i]);
            }
        }
    }

    // thread unlock
    pthread_mutex_unlock(&lock);
}

void *dosyaSil(char *args)
{
    // thread lock
    pthread_mutex_lock(&lock);
    struct parameters *parameters = args;
    // dosyanın adı argümandan bulunur
    char *dosya_adi = parameters->arg1;
    printf("Dosya Adi : %s\n", dosya_adi);
    // kontrol != -1 ise dosya vardır ve silinir
    if (dosyaKontrol(dosya_adi) != -1)
    {
        dosya_listesi[dosyaKontrol(dosya_adi)][0] = '\0';
        remove(dosya_adi);
        strcpy(response, "Dosya silindi!\n");
    }
    else
    {
        strcpy(response, "Belirtilen dosya bulunamadi.\n");
    }
    // thread unlock
    pthread_mutex_unlock(&lock);
}

void *dosyaOku(char *args)
{
    // thread lock
    pthread_mutex_lock(&lock);
    struct parameters *parameters = args;
    // dosyanın adı argümandan bulunur
    char *dosya_adi = parameters->arg1;
    char icerik[128];
    printf("Dosya Adi : %s\n", dosya_adi);
    // kontrol != -1 ise dosya vardır ve okunur
    if (dosyaKontrol(dosya_adi) != -1)
    {
        int i = 0;
        FILE *fptr = fopen(dosya_adi, "r");
        while ((c = fgetc(fptr)) != EOF)
        {
            icerik[i] = c;
            i++;
        }
        icerik[i - 1] = '\0';
        fclose(fptr);
        strcpy(response, icerik);
    }
    else
    {
        strcpy(response, "Belirtilen dosya bulunamadi.\n");
    }
    // thread unlock
    pthread_mutex_unlock(&lock);
}

void *dosyayaYaz(char *args)
{
    // thread lock
    pthread_mutex_lock(&lock);
    struct parameters *parameters = args;
    // dosyanın adı argüman 1'den bulunur
    char *dosya_adi = parameters->arg1;
    // yazılacak input argüman 2'den bulunur
    char *input = parameters->arg2;
    printf("Dosya Adi : %s\n", dosya_adi);
    // kontrol != -1 ise dosya vardır ve içine yazılır
    if (dosyaKontrol(dosya_adi) != -1)
    {
        // dosya yazma işlemleri
        FILE *file = fopen(dosya_adi, "a+");
        if (file == NULL)
        {
            perror("fopen failed");
        }

        fprintf(file, "%s\n", input);

        if (fclose(file) == EOF)
        {
            perror("fclose failed");
            // return;
        }
        printf("Input : %s\n", input);
        strcpy(response, "Input dosyaya yazildi!\n");
    }
    else
    {
        strcpy(response, "Belirtilen dosya bulunamadi.\n");
    }
    // thread unlock
    pthread_mutex_unlock(&lock);
}

int main()
{
    char **command;
    void *status;
    int file_read;
    int resp = 0;
    char *myfifo = "/tmp/file_manager_named_pipe";
    char buffer[128];
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    while (1)
    {
        file_read = open(myfifo, O_RDONLY);
        read(file_read, buffer, 128);
        // girilen komutlar bölüştürme metoduna yollanır
        command = komutBolustur(buffer);

        struct parameters parameters;
        // komutlar argümanlara atanır
        parameters.arg1 = command[1];
        parameters.arg2 = command[2];
        // client oluşturma, init işlemi
        if (strcmp(command[0], "init") == 0)
        {
            client_size++;
            printf("%d. Client olusturuldu!\n", client_size);
        }
        // create komutu çağırılırsa pthread ve dosya oluşturulur
        else if (strcmp(command[0], "create") == 0)
        {
            pthread_create(&threads[0], NULL, dosyaOlustur, &parameters);
            resp = 1;
        }
        // delete komutu çağırılırsa pthread oluşturulur ve belirtilen dosya silinir
        else if (strcmp(command[0], "delete") == 0)
        {
            pthread_create(&threads[1], NULL, dosyaSil, &parameters);
            resp = 1;
        }
        // write komutu çağırılırsa pthread oluşturulur ve belirtilen input o dosyaya yazılır
        else if (strcmp(command[0], "write") == 0)
        {
            pthread_create(&threads[2], NULL, dosyayaYaz, &parameters);
            resp = 1;
        }
        // read komutu çağırılırsa pthread oluşturulur ve belirtilen dosya okunur
        else if (strcmp(command[0], "read") == 0)
        {
            pthread_create(&threads[3], NULL, dosyaOku, &parameters);
            resp = 1;
        }
        // exit komutu çağırılırsa client işlemi sonlandırılır
        else if (strcmp(command[0], "exit") == 0)
        {
            strcpy(response, "Client islemi sonlandirildi!\n");
            resp = 1;
            client_size--;
            // bütün clientlar sonlandırılmışsa aynı şekilde manager da sonlandırılır
            if (client_size == 0)
            {
                printf("Program sonlandirildi!\n");
                file_read = open(myfifo, O_WRONLY);
                write(file_read, response, sizeof(response));
                close(file_read);
                exit(0);
            }
        }
        else
        {
            // yanlış bir komut girilirse client tarafına response gönderilir
            strcpy(response, "Hatali Komut Girdiniz!\n***Create***\n***Delete***\n***Write***\n***Read***\n***Exit***\n");
            resp = 1;
        }

        for (int i = 0; i < 5; i++)
        {
            pthread_join(threads[i], &status);
        }

        if (resp == 1)
        {
            file_read = open(myfifo, O_WRONLY);
            write(file_read, response, sizeof(response));
            close(file_read);
        }
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    exit(0);
}
