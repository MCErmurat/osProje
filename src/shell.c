/**
#* @course		İşletim Sistemleri
#* @assignment		Proje
#* @group		51
#* @author Y245012115 	Mürvet Esmer
#* @author Y245012116 	Oktay Esmer
#* @author G221210381 	Mehmet Can Ermurat
#* @author G221210372 	Mustafa Sait Karadağ
#* @author Y245060024 	Nihan Hüsna Kılıç Beşik
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 1024    // Maksimum satır uzunluğu
#define MAX_ARGS 64      // Maksimum komut argümanı sayısı

int background_processes = 0;  // Arka planda çalışan işlem sayısı

// Arka planda çalışan işlemleri yönetmek için sinyal işleyicisi
void handle_background_process(int sig) {
    int status;
    pid_t pid;
    
    // Arka planda biten işlemleri bekleyip, dönen değerleri yazdır
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[%d] retval: %d\n", pid, WEXITSTATUS(status));
        background_processes--;  // Arka planda bir işlem bittiğinde sayacı azalt
    }
}

int main() {
    char line[MAX_LINE];       // Kullanıcıdan alınacak komut satırı
    char *args[MAX_ARGS];      // Komutun parçalanmış argümanları
    struct sigaction sa;       // Sinyal işleyicisi için yapı
    
    // Signal handler kurulumu
    sa.sa_handler = handle_background_process;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);  // Arka planda biten işlemler için SIGCHLD sinyalini dinle

    while (1) {
        printf("> ");  // Komut satırı istemi
        fflush(stdout);  // Çıktıyı hemen yazdır

        // Kullanıcıdan komut satırını al
        if (fgets(line, MAX_LINE, stdin) == NULL) break;  // EOF veya hata durumunda çık
        
        // Satır sonundaki newline karakterini kaldır
        line[strcspn(line, "\n")] = 0;
        
        // Quit komutu kontrolü
        if (strcmp(line, "quit") == 0) {
            // Arka planda çalışan tüm işlemler bitene kadar bekle
            while (background_processes > 0) {
                pause();  // Sinyal bekle
            }
            exit(0);  // Çıkış
        }

    