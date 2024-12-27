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
        // Komutları noktalı virgülle ayır
        char *commands[MAX_ARGS];
        int cmd_count = 0;
        
        char *command = strtok(line, ";");
        while (command != NULL && cmd_count < MAX_ARGS - 1) {
            commands[cmd_count++] = command;
            command = strtok(NULL, ";");
        }
        commands[cmd_count] = NULL;

    // Her komutu sırayla çalıştır
        for (int i = 0; i < cmd_count; i++) {
            char *token;
            int j = 0;
            char *args[MAX_ARGS];
            
            // Komutun parçalanması (tokenize)
            token = strtok(commands[i], " ");
            while (token != NULL && j < MAX_ARGS - 1) {
                args[j++] = token;
                token = strtok(NULL, " ");
            }
            args[j] = NULL;

            // Arka planda çalışma kontrolü
            int background = 0;
            if (j > 0 && strcmp(args[j-1], "&") == 0) {
                background = 1;  // "&" varsa arka planda çalışacak
                args[--j] = NULL;  // "&" karakterini args'tan çıkar
            }

            // I/O yönlendirme ve pipe işlemleri için değişkenler
            int input_fd = -1, output_fd = -1;
            int pipe_count = 0;
            int pipe_pos[MAX_ARGS];
            
            // I/O yönlendirme ve pipe pozisyonlarını bul
            for (int k = 0; k < j; k++) {
                if (strcmp(args[k], "<") == 0) {  // Girdi yönlendirme
                    if (k + 1 < j) {
                        input_fd = open(args[k+1], O_RDONLY);  // Dosyayı aç
                        if (input_fd == -1) {
                            printf("%s giriş dosyası bulunamadı.\n", args[k+1]);
                            continue;
                        }
                        args[k] = NULL;
                        k++;
                    }
                } else if (strcmp(args[k], ">") == 0) {  // Çıktı yönlendirme
                    if (k + 1 < j) {
                        output_fd = open(args[k+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);  // Dosyayı aç veya oluştur
                        args[k] = NULL;
                        k++;
                    }
                } else if (strcmp(args[k], "|") == 0) {  // Pipe
                    pipe_pos[pipe_count++] = k;  // Pipe pozisyonlarını kaydet
                    args[k] = NULL;
                }
            }
            if (pipe_count == 0) {
                // Tek komut durumu (pipe yok)
                pid_t pid = fork();  // Yeni bir çocuk işlem oluştur
                
                if (pid == 0) {
                    // Çocuk proses
                    if (input_fd != -1) {
                        dup2(input_fd, STDIN_FILENO);  // Giriş yönlendirme
                        close(input_fd);
                    }
                    if (output_fd != -1) {
                        dup2(output_fd, STDOUT_FILENO);  // Çıktı yönlendirme
                        close(output_fd);
                    }
                    
                    execvp(args[0], args);  // Komutu çalıştır
                    perror("execvp");  // Eğer execvp başarısız olursa hata mesajı yazdır
                    exit(1);
                } else {
                    // Ebeveyn proses
                    if (!background) {
                        waitpid(pid, NULL, 0);  // Arka planda değilse, çocuk işlemin bitmesini bekle
                    } else {
                        background_processes++;  // Arka planda çalışacaksa sayacı artır
                    }
                }
            } else {
                // Pipe durumu
                int pipes[pipe_count][2];  // Pipe'lar için dizi
                for (int k = 0; k < pipe_count; k++) {
                    pipe(pipes[k]);  // Her pipe için bir kanal oluştur
                }

                pid_t pids[pipe_count + 1];  // Pipe'lar için pid dizisi
                char **cmd = args;
                
                for (int k = 0; k <= pipe_count; k++) {
                    pids[k] = fork();  // Her komut için yeni bir çocuk işlem oluştur
                    
                    if (pids[k] == 0) {
                        // İlk komut için giriş yönlendirme
                        if (k == 0 && input_fd != -1) {
                            dup2(input_fd, STDIN_FILENO);  // Giriş yönlendirme
                            close(input_fd);
                        }
                        
                        // Son komut için çıkış yönlendirme
                        if (k == pipe_count && output_fd != -1) {
                            dup2(output_fd, STDOUT_FILENO);  // Çıktı yönlendirme
                            close(output_fd);
                        }
                        
                        // Pipe bağlantıları
                        if (k > 0) {
                            dup2(pipes[k-1][0], STDIN_FILENO);  // Önceki pipe'ın çıkışını al
                        }
                        if (k < pipe_count) {
                            dup2(pipes[k][1], STDOUT_FILENO);  // Şu anki pipe'ın girişine yaz
                        }
                        
                        // Kullanılmayan pipe'ları kapat
                        for (int l = 0; l < pipe_count; l++) {
                            close(pipes[l][0]);
                            close(pipes[l][1]);
                        }
                        
                        execvp(cmd[0], cmd);  // Komutu çalıştır
                        perror("execvp");  // Eğer execvp başarısız olursa hata mesajı yazdır
                        exit(1);
                    }
                    
                    // Bir sonraki komuta geç
                    if (k < pipe_count) {
                        cmd = &args[pipe_pos[k] + 1];
                    }
                }
                
                // Ebeveyn tüm pipe'ları kapatır
                for (int k = 0; k < pipe_count; k++) {
                    close(pipes[k][0]);
                    close(pipes[k][1]);
                }
                
                // Tüm çocuk prosesleri bekle
                if (!background) {
                    for (int k = 0; k <= pipe_count; k++) {
                        waitpid(pids[k], NULL, 0);  // Çocuk işlemlerin bitmesini bekle
                    }
                } else {
                    background_processes += pipe_count + 1;  // Arka planda çalışan işlem sayısını artır
                }
            }