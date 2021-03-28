#ifndef STRUCTURE_H
#define STRUCTURE_H

// slaves terminal
#define SLAVES 4

// alarm mode
#define DISABLED 0   
#define ENABLED 1

// alarm sound 
#define OFF 0   
#define ON 1

// slave mode => definisce se la finestra/porta (slave) è aperta o chiusa
#define CLOSE 0
#define OPEN 1

extern int TERMINALS[]; // input array dei pin dei pulsanti che simulano gli slaves
extern int LED[];   // output array dei led che indicano lo stato degli slaves (aperti/chiusi)
extern int BUZZER;  // pin buzzer di allarme
extern int ALARM_LED; // pin led di allarme
extern int MOV_TRIG; // Output pin HC-SR04 sensore rilevamento movimenti
extern int MOV_ECHO; // Input pin HC-SR04 sensore rilevamento movimenti


// Load Wi-Fi library
#include <WiFi.h>
#include <PubSubClient.h>

// Network credentials
extern const char* ssid;
extern const char* password;
extern const char* mqtt_server;
extern const char* clientid;
extern char topic_id[30];   // copia del topic in arrivo
extern char topic_payload[60];   // copia del payload in arrivo
extern boolean ACK;   // true se c'è un messaggio di ack da mandare


extern const char* topic_alarm_mode_on; // Pulsante attivazione allarme 
extern const char* topic_alarm_mode_off;  // Pulsante disattivazione allarme
extern const char* topic_alarm_sound;  // Pulsante disattivazione suono allarme
extern const char* topic_alarm_sound_text;  // Tab di notifica stato suono allarme 
extern const char* topic_alarm_received;  // Tab di ricezione, ACK dei comandi inviati da APP
extern const char* topic_open_slaves; // Tab contatore slave aperti
extern const char* topic_motion_detection_code; // Pulsante disabilitazione timer di inserimento codice

// HC-SR04 Distance sensor 
#include <Ultrasonic.h>

extern int curr_distance;
extern int last_distance;
extern bool first_read;

extern void callback(char* topic, byte* payload, unsigned int length);

extern WiFiClient espClient;
extern PubSubClient client;

// Shared Resource
struct home_state {
  bool slave_state[SLAVES];  // slave state: OPEN/CLOSE
  bool alarm_mode;   // mode: DISABLED/ENABLED
  bool alarm_sound;    // ON/OFF per far suonare l'allarme 
  int open_slaves;    // contatore finestre aperte
};

#endif  
