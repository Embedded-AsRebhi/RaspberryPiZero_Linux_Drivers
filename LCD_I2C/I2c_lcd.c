#include <stdio.h>
#include <pigpio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define I2C_BUS 1
#define I2C_ADDR 0x20



 // Définitions des commandes LCD 
 // this a differnet commande to communicate with the LCD 
enum {
    LCD_CLEAR = 0x01,
    LCD_RETURNHOME = 0x02,
    LCD_ENTRYMODESET = 0x04,
    LCD_DISPLAYCONTROL = 0x08,
    LCD_CURSORSHIFT = 0x10,
    LCD_FUNCTIONSET = 0x20,
    
    LCD_4BITMODE = 0x00, //send 0 to write
    LCD_2LINE = 0x08,
    LCD_DISPLAYON = 0x04,
    LCD_CURSORON = 0x02,
    LCD_BLINKON = 0x01,
    LCD_ENTRYLEFT = 0x02,
    LCD_ENTRYSHIFTINCREMENT = 0x01,
    LCD_BACKLIGHT = 0x08 
};

// Définitions des broches I2C
enum {
    _RS = 0x10,   // P4 :: Register Select
    _EN = 0x40,   // P6 :: Enable
    _DATA_PINS = 0x0F,  // P0-P3 for data
    
};

int lcd_handle;

//Function to write on the LCD 
void lcd_write4bits(uint8_t value, uint8_t mode) {
    // Construction de l'octet à envoyer
    uint8_t data = (value & 0x0F) | (mode ? _RS : 0) ;
    // Impulsion Enable
    i2cWriteByte(lcd_handle, data | _EN);
    gpioDelay(1);  // 1µs
    i2cWriteByte(lcd_handle, data & ~_EN);
    gpioDelay(50); // 50µs
}

// function to send the data
void lcd_send(uint8_t value, uint8_t mode) {
    // Envoi des 4 bits de poids fort d'abord
    lcd_write4bits(value >> 4, mode);
    // Puis des 4 bits de poids faible
    lcd_write4bits(value, mode);
}

//Function to send a command 
void lcd_command(uint8_t cmd) {
    lcd_send(cmd, 0);
}

void lcd_write(char c) {
    lcd_send(c, 1);
}

void lcd_init() {
    // Initialisation en mode 4 bits 
    gpioDelay(50000); // Attente 50ms
    
    // initialisation spécifique pour demarrer LCD 
    lcd_write4bits(0x03, 0);  // Mode 8 bits
    gpioDelay(4500);          // Attente 4.5ms
    lcd_write4bits(0x03, 0);  // Mode 8 bits
    gpioDelay(150);           // Attente 150µs
    lcd_write4bits(0x03, 0);  // Mode 8 bits
    gpioDelay(150);
    lcd_write4bits(0x02, 0);  // Passage en mode 4 bits
    
    // Configuration du LCD
    lcd_command(LCD_FUNCTIONSET | LCD_2LINE | LCD_4BITMODE);
    gpioDelay(60);
    lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
    gpioDelay(60);
    lcd_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT);
    gpioDelay(60);
    lcd_command(LCD_CLEAR);
    gpioDelay(2000);  // Attente 2ms pour le clear
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t offset = (row == 0) ? 0x80 : 0xC0;
    lcd_command(offset + col);
    gpioDelay(60);
}

void lcd_print(const char *str) {
    while (*str) {
        lcd_write(*str++);
        gpioDelay(60);
    }
}

// funcion to retrieve the telp and Humidity 
int get_Temp_Hum(int *temp, int *hum) {
    int fd = open("/dev/dht", O_RDONLY);
    if (fd == -1) {
        perror("Erreur d'ouverture /dev/dht");
        return -1;
    }
    char buff[20];  // Assez grand pour contenir "xx yy"
    int len = read(fd, buff, sizeof(buff) - 1);
    if (len <= 0) {
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
void display_temp_hum(int temp, int hum){
    char line1[16], line2[16];
    lcd_set_cursor(0, 0);
    snprintf(line1, sizeof(line1), "Temp : %d °C" ,temp);
    lcd_print(line1);
    lcd_set_cursor(1, 0);
    snprintf(line2, sizeof(line2), "Hum : %d %%" ,hum);
    lcd_print(line2);

}   

// main Function 
int main() {
    int temp, hum;
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Erreur d'initialisation pigpio\n");
        return 1;
    } 
    printf("Initialise done\n");
    lcd_handle = i2cOpen(I2C_BUS, I2C_ADDR, 0);
    printf("open done\n");
    if (lcd_handle < 0) {
        fprintf(stderr, "Erreur ouverture I2C\n");
        gpioTerminate();
        return 1;
    }
    printf("LCD handle  done\n");
    lcd_init();
    printf("init done\n");
    lcd_set_cursor(0, 0);
    printf("set cursor done\n");
    lcd_print(" Welcoooome ");
    sleep(2);
    lcd_command( LCD_CLEAR);
     
    if (get_Temp_Hum(&temp, &hum) == 0) {
        display_temp_hum(temp, hum);
    } else {
        lcd_print("Erreur capteur");
    }
    sleep(5);
    i2cClose(lcd_handle);
    printf("close done\n");
    gpioTerminate();
    return 0;
}