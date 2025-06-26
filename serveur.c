#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include<signal.h>


int interval_log = 60; //veach 60 seconds save the TEmp and Hum
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // initialise un mutex to save the temp and hum in the log file 
pthread_mutex_t lock_readdist = PTHREAD_MUTEX_INITIALIZER; // initialise un mutex for read a distance (using ultrasonic drivers)


int etat_servo = 0; // 0 close, 160 open
volatile sig_atomic_t runing =1;
void* log_thread(void* arg);
int get_Temp_Hum(int *temp, int *hum);
void get_histo(FILE* file, int cli, char* buf);
void* dist_thread(void* arg);
void* lcd_display_thread(void* arg);
void state_led();
int get_dist(char *dist);
void control_servo(char *buf) ;

//handle les signal 
// generate the signale to stop :: Ctrl+C 
int lcd_handle;
void handle_signal(int sig) {
    printf("\nReçu signal %d. Fermeture du serveur...\n", sig);
    runing = 0;
}
int main() {


    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
   
    int temp, hum;
    char buf[64]; 
    char dist[8] = {0};
    int len;
    int cli;
    
    pthread_t thread1, threaddist ;
    pthread_create(&thread1, NULL, log_thread, NULL); //log thread to save temp and hum
    pthread_create(&threaddist, NULL, dist_thread, NULL); // thread to controle the distance , if if less than 10 cm , a green leed will turn on alse turn off
    
    //Config et launch the serveur 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }
    //This function to free the port after a stop request
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;    
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Erreur bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Mettre le serveur en attente d’une connexion
    listen(sock, 5);
    printf("Serveur en attente de connexion...\n");

   // infinite loop as long as the server is launched
    while (runing) {

        cli = accept(sock, NULL, NULL);
        if (cli < 0) {
            perror("Erreur accept");
            continue;
        }
        
        // here will wait the command request to launch a suitable structure
        while ((len = read(cli, buf, sizeof(buf) - 1)) > 0) {
            buf[len] = '\0';
            printf("Commande reçue : %s\n", buf);

            if (get_Temp_Hum(&temp, &hum) == -1) {
                strcpy(buf, "ERR");
            } else if (strcmp(buf, "TEMP") == 0) {
                sprintf(buf, "%d", temp);
            } else if (strcmp(buf, "HUM") == 0 || strcmp(buf, "Hum") == 0) {
                sprintf(buf, "%d", hum);
            } else if (strncmp(buf, "SET_INTERVAL", 12) == 0) {
            // buf + 13 skips the first 13 characters to go directly to the number
                int new_interval = atoi(buf + 13);
                pthread_mutex_lock(&lock);
                interval_log = new_interval;  
                pthread_mutex_unlock(&lock);
                strcpy(buf, "OK");
            } else if (strcmp(buf, "Histo") == 0) {
                FILE* log = fopen("dht.log", "r");
                get_histo(log, cli, buf);
            
            } else if (strcmp(buf, "LED") == 0) {
               
                state_led();
                strcpy(buf, "OK");
                
            } else if (strcmp(buf, "Dist") == 0) {
                if (get_dist(dist) == 0) {
                    write(cli, dist, strlen(dist)); 
                } else {
                    strcpy(buf, "Erreur lecture distance");
                    write(cli, buf, strlen(buf));
                }

            } else if (strcmp(buf, "Serveur") == 0) {
                strcpy(buf, "ok\n");
                write(cli, buf, strlen(buf)); 
                close(cli);
                
                runing = 0;
                break; // sort de la boucle de communication
            } else if (strcmp(buf, "Servo") == 0) {
                control_servo(buf);     
            }else {
                strcpy(buf, "Commande inconnue");
            }
            // send the response
            write(cli, buf, strlen(buf));
        }
    
    close(cli);
    printf("Client déconnecté\n");
    
    }
    
   
    pthread_join(thread1, NULL);
    pthread_join(threaddist, NULL);
    close(sock);
    pthread_mutex_destroy(&lock_readdist);
    return EXIT_SUCCESS;
}

//function to turn on the red led if temperature more than 20 
void set_led(int state){
    int fd =open("/dev/ledrouge", O_WRONLY);
    if(fd == -1){
        printf("Not possible de open file temp if sup 20\n");
    }
    else {
        char cmd = state ? '1' : '0';
        //pthread_mutex_lock(&lock_writeled);
        write(fd, &cmd, 1);
        //pthread_mutex_unlock(&lock_writeled);
        close(fd);
    }
}

//get the distance from /dev/ultrasonic 
int get_dist(char *dist) {

    pthread_mutex_lock(&lock_readdist);
    int fd = open("/dev/ultrasonic", O_RDONLY);
    if(fd < 0) {
        perror("Error to open file ultrasonic\n");
        pthread_mutex_unlock(&lock_readdist);
        return -1;
    }
    
    int len = read(fd,dist,sizeof(dist)-1);
    close(fd);
    pthread_mutex_unlock(&lock_readdist);
   
    if(len < 0) {
        printf("Error to read ultrasonic file\n");
        return -1;
    }
    dist[len] = '\0';
    return 0;
}

//function to turn on the green led if distance less than 10 cm
void set_leddist(int state){

    int fd =open("/dev/ledvert", O_WRONLY);
    if(fd == -1){
        perror("Not possible de open file dist if sup 10\n");
    }
    else {
        char cmd = state ? '1' : '0';
        //pthread_mutex_lock(&lock_writeled);
        write(fd, &cmd, 1);
        //pthread_mutex_unlock(&lock_writeled);
        close(fd);
    }
}


//Read the temp and Hum from /dev/dht
int get_Temp_Hum(int *temp, int *hum) {
    int fd = open("/dev/dht", O_RDONLY);
    if (fd == -1) {
        perror("Erreur d'ouverture /dev/dht");
        return -1;
    }
    char buff[20];  
    int len = read(fd, buff, sizeof(buff) - 1);
    if (len < 0) {
        perror("Erreur de lecture /dev/dht");
        close(fd);
        return -1;
    }
    buff[len] = '\0';
    if (sscanf(buff, "%d %d", temp, hum) != 2) {
        fprintf(stderr, "Format invalide dans /dev/dht: %s\n", buff);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}


//This function to copy the log file(temp and hum) from server to the client
void* log_thread(void* arg) {
    
    while (runing) {
        int temp, hum;
        if (get_Temp_Hum(&temp, &hum) == 0) {
            system("touch dht.log");
            FILE *log = fopen("dht.log", "a");
            if (log) {
                time_t now = time(NULL);
                char* timestr = ctime(&now);
                timestr[strlen(timestr) - 1] = 0;
                fprintf(log, "[%s] : Temp : %d , Hum : %d\n ", timestr, temp, hum);
                fclose(log);
            }
        }
        set_led(temp > 20 ? 1 : 0);
        pthread_mutex_lock(&lock);
        int wait_time = interval_log;    
        pthread_mutex_unlock(&lock);
        for (int i = 0; i < wait_time && runing; ++i) {
            sleep(1);
        }
    }
    set_led(0);
    return NULL;
}

// this function to turn on or turn off the bleu led from client  
void state_led(){
    int fd = open("/dev/ledbleu", O_RDWR);
    char state[8]={0}; 
    if (fd == -1 ){
        printf("Error to open file led1\n");
    }
    ssize_t readled2 = read(fd,state,sizeof(state)-1);
    if(readled2 <= 0){
        printf("error de read\n");
        close(fd);   
    }
    state[strcspn(state, "\r\n")] = 0;
    printf("Etat actuel de la LED : %s\n", state);
    char new_state[2] = {0};
    if(strcmp(state,"OFF") == 0){
        strcpy(new_state , "1");
    } else if (strcmp(state,"ON") == 0) {
        strcpy(new_state , "0");
    } else {
        fprintf(stderr, "État LED inconnu : %s\n", state);
        close(fd);
    }
 //nettoyer le fichier avant d'ecrire et mettre le cursor en avant 
    lseek(fd, 0, SEEK_SET);
    if (write(fd, new_state, 1) <= 0) {
        perror("Erreur écriture");
        close(fd);
    } else {
        printf("LED changed to : %s\n", new_state);
    }

    close(fd);
}



// display the historique
void get_histo(FILE* file, int cli, char* buf) {
    char line[256];
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            write(cli, line, strlen(line));
            usleep(10000);  
    }
    write(cli, "__END__", strlen("__END__"));
    fclose(file);
    } else {
        strcpy(buf, "no histo\n");
        write(cli, buf, strlen(buf));
    } 
}

void* dist_thread(void* arg) {
    char dist[8];
    while (runing) {
        if (get_dist(dist) == 0) {
            int distance = atoi(dist);
            set_leddist(distance < 10 ? 1 : 0);
        }
        sleep(1);  
    }
    set_leddist(0);
    return NULL;
}
 
// this function to turn on the servomottor to 160° or 0°
void control_servo(char *buf) {
    static int etat_servo = 0;  // close 0, open 160
    int angle = (etat_servo == 0) ? 160 : 0;
    etat_servo = angle;
    int fd = open("/dev/servomotor", O_WRONLY);
    if (fd >= 0) {
        char cmd[8];
        snprintf(cmd, sizeof(cmd), "%d\n", angle);
        write(fd, cmd, strlen(cmd));
        close(fd);
        sprintf(buf, "Servo positionne : %d", angle);
    } else {
        perror("Erreur ouverture servomotor");
        strcpy(buf, "Erreur servo");
    }

}
