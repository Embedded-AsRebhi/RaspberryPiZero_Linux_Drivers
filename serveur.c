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
    char buf[64];  // taille augmentée pour SET_INTERVAL etc.
    char dist[8] = {0};
    int len;
    int cli;
     // Ici on a fait dans un runing pour qu'il accepte plusieur request 
    //signal(SIGINT, handle_signal);

     // fin de lancement de seveur 
    // create a thread afin de faire un file.log et enregidtre temp et hum
        
    pthread_t thread1, threaddist ;
    pthread_create(&thread1, NULL, log_thread, NULL);
    pthread_create(&threaddist, NULL, dist_thread, NULL);
    // Lancer le thread LCD
    //pthread_create(&thread_lcd, NULL, lcd_display_thread, NULL);
        
    //config et lancement de sevserveur 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }
    //pour debloqué le port 
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
    // autorise jusqu’à 5 clients en attente (dans la file) s’il n’y a pas encore eu de accept()”.
    listen(sock, 5);
    printf("Serveur en attente de connexion...\n");

   
    while (runing) {

        cli = accept(sock, NULL, NULL);
        if (cli < 0) {
            perror("Erreur accept");
            continue;
        }
        //printf("Client connected\n");
        // len == 0 : FIN DE FICHIER ou client a fermé le socket 
        // va lire le request de client
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
                /// buf + 13 saute les 13 premiers caractères pour aller directement au nombre.
                // sprintf(commandInterval, "SET_INTERVAL %d",new_int);
                int new_interval = atoi(buf + 13);
                pthread_mutex_lock(&lock);
                interval_log = new_interval;  // Écriture
                pthread_mutex_unlock(&lock);
                strcpy(buf, "OK");
            } else if (strcmp(buf, "Histo") == 0) {
                FILE* log = fopen("dht.log", "r");
                get_histo(log, cli, buf);
                //continue;
            } else if (strcmp(buf, "LED") == 0) {
               
                state_led();
                strcpy(buf, "OK");
                //continue;
            } else if (strcmp(buf, "Dist") == 0) {
               
                //char dist[32] = {0};
                if (get_dist(dist) == 0) {
                    write(cli, dist, strlen(dist)); 
                } else {
                    strcpy(buf, "Erreur lecture distance");
                    write(cli, buf, strlen(buf));
                }
                //strcpy(buf, "OK");
                //continue;
            } else if (strcmp(buf, "Serveur") == 0) {
              
                //printf("Bye Bye \n");
                strcpy(buf, "ok\n");
                write(cli, buf, strlen(buf));  // envoyer une réponse au client
                close(cli);
                //should_close = 0;  // le close est reporté après la boucle
                runing = 0;
                break; // sort de la boucle de communication
            } else if (strcmp(buf, "Servo") == 0) {
                control_servo(buf);     
            }else {
                strcpy(buf, "Commande inconnue");
            }
            

            // envoyer la réponse
            write(cli, buf, strlen(buf));
        }
    
    close(cli);
    //printf("Client déconnecté\n");
    
    }
    
    //runing =0;
    pthread_join(thread1, NULL);
    pthread_join(threaddist, NULL);
    //pthread_join(thread_lcd, NULL);
    //pthread_join(threadservo, NULL);
    close(sock);
    //pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lock_readdist);
    return EXIT_SUCCESS;
}

//fonction pour allumer led rouge si temp sup a 20 
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
// lecture de la distance de dev/ultrasonic 
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
    //printf("Read %d bytes: %s\n", len, dist);
    return 0;
}

// Fonction pour allumer led vert si distance inf 10 sinon va fermer 
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


// Lecture de la température et humidité depuis /dev/dht
// Format attendu : "30 45" (température humidité)

int get_Temp_Hum(int *temp, int *hum) {
    int fd = open("/dev/dht", O_RDONLY);
    if (fd == -1) {
        perror("Erreur d'ouverture /dev/dht");
        return -1;
    }
    char buff[20];  // Assez grand pour contenir "xx yy"
    int len = read(fd, buff, sizeof(buff) - 1);
    if (len < 0) {
        perror("Erreur de lecture /dev/dht");
        close(fd);
        return -1;
    }

    buff[len] = '\0';

    // Extraction des deux valeurs
    // sscanf lit une data depuis une chaîne de caractères (buff)
    // sscanf diff de scanf, sscanf lit dans buffer et scanf lit de clavier
    if (sscanf(buff, "%d %d", temp, hum) != 2) {
        fprintf(stderr, "Format invalide dans /dev/dht: %s\n", buff);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


// cette fonction pour copie le log file de serveur vers ici .

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
        // le wait_time peut accéder par le serveur thread et cette thread lui-même.
        pthread_mutex_lock(&lock);
        int wait_time = interval_log;    // Lecture
        //sleep(interval_log);  // Dort avec le verrou ! Bloque les autres threads.
        pthread_mutex_unlock(&lock);
        for (int i = 0; i < wait_time && runing; ++i) {
            sleep(1);
        }
        //sleep(wait_time);
    }
    set_led(0);
    return NULL;
}

// cette fonction pour controler la led a distance et l'allumer si in esp allumer 
void state_led(){
    int fd = open("/dev/ledbleu", O_RDWR);
    char state[8]={0}; 
    if (fd == -1 ){
        printf("Error to open file led1\n");
        //return 0;
    }
    //printf("file opend\n");
    ssize_t readled2 = read(fd,state,sizeof(state)-1);
    if(readled2 <= 0){
        printf("error de read\n");
        close(fd);   
        //return 0; 
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
        //return 0;
    }
 //nettoyer le fichier avant d'ecrire et mettre le cursor en avant 
    lseek(fd, 0, SEEK_SET);
    if (write(fd, new_state, 1) <= 0) {
        perror("Erreur écriture");
        close(fd);
        //return 0;
    } else {
        printf("LED changed to : %s\n", new_state);
    }

    close(fd);
}



// Affiche l'historique
void get_histo(FILE* file, int cli, char* buf) {
    char line[256];
    if (file) {
        // fgets va lire de fichier log
        while (fgets(line, sizeof(line), file)) {
            write(cli, line, strlen(line));
            usleep(10000); // petite pause en micro pour ne pas saturer le buffer 
    }
    write(cli, "__END__", strlen("__END__"));
    fclose(file);
    } else {
        strcpy(buf, "no histo\n");
        //buf ne contient plus rien d'utile à ce moment-là,donc il faut ce write 
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
        sleep(1);  // Vérifie la distance chaque seconde
    }
    set_leddist(0);
    return NULL;
}

void control_servo(char *buf) {
    //pthread_mutex_lock(&lock_servo);

    static int etat_servo = 0;  // fermé 0, ouvert 180
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

    //pthread_mutex_unlock(&lock_servo);
}
