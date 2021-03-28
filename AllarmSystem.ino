#include "structure.h"

// TaskHandles
TaskHandle_t Handle_TaskSlave[SLAVES];
TaskHandle_t Handle_TaskAlarm;
TaskHandle_t Handle_TaskMain;
TaskHandle_t Handle_TaskConnection;
TaskHandle_t Handle_TaskMovementDetection;

// Timers

TimerHandle_t Timer_MovementDetection;
TimerHandle_t Timer_code;

// Semaphores
SemaphoreHandle_t mutex_home;
SemaphoreHandle_t mutex_alarm;
SemaphoreHandle_t mutex_movement;

//Task Definitions
void TaskSlave(void* pvParameters);
void TaskAlarm(void* pvParameters);
void TaskMain(void* pvParameters);
void TaskConnection(void* pvParameters);
void TaskMovementDetection(void* pvParameters);

// shared struct resource
struct home_state home;

void setup() {

  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  pinMode(ALARM_LED, OUTPUT);
  

  /* Inizializzazione struttura e semafori*/
  for (int i = 0; i < SLAVES; i++)
  {
    home.slave_state[i] = CLOSE;
    pinMode(TERMINALS[i], INPUT);
    pinMode(LED[i], OUTPUT);
  }
  home.alarm_mode = ENABLED;
  home.alarm_sound = OFF;
  home.open_slaves = 0;
  mutex_home = xSemaphoreCreateCounting(1, 1);
  mutex_alarm = xSemaphoreCreateCounting(1, 0);
  mutex_movement = xSemaphoreCreateCounting(1, 0);

  Timer_MovementDetection = xTimerCreate("Movement sensor Timer", pdMS_TO_TICKS(2000), pdTRUE,(void *) 0, timer_callback);
  Timer_code = xTimerCreate("Alarm code Timer", pdMS_TO_TICKS(10000), pdFALSE,(void *) 1, timer_callback);

  Serial.println("Creating slave tasks..");
  delay(200);   
  
    /* Creazione N task di slave */
  for (int i = 0; i < SLAVES; i++)
  { 
    char taskName[15];
    snprintf(taskName, sizeof(taskName), "Task Slave %d", i);
       
    xTaskCreate(
    TaskSlave,
    taskName,     
    2500,       
    (void *) i,        
    10,           
    &Handle_TaskSlave[i]
    );
    delay(500);
  }

    /* Creazione task di allarme */
  xTaskCreatePinnedToCore(
    TaskAlarm,   /* Task function. */
    "Alarm Task",     /* name of task. */
    2000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    15,           /* priority of the task */
    &Handle_TaskAlarm,      /* Task handle to keep track of created task */
    0);
    delay(200);
        
    /* Creazione task di connessione */
  xTaskCreatePinnedToCore(
    TaskConnection,   /* Task function. */
    "Connection Task",     /* name of task. */
    2300,       /* Stack size of task */
    NULL,        /* parameter of the task */
    10,           /* priority of the task */
    &Handle_TaskConnection,      /* Task handle to keep track of created task */
    1);
    delay(200);
  
    /* Creazione task di gestione sistema */
  xTaskCreatePinnedToCore(
    TaskMain,   /* Task function. */
    "Main Task",     /* name of task. */
    1000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    12,           /* priority of the task */
    &Handle_TaskMain,      /* Task handle to keep track of created task */
    1);
    delay(200);

    /* Creazione task di rilevamento movimenti */
  xTaskCreatePinnedToCore(
    TaskMovementDetection,   /* Task function. */
    "Movement Detection Task",     /* name of task. */
    1000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    10,           /* priority of the task */
    &Handle_TaskMovementDetection,      /* Task handle to keep track of created task */
    1);
    delay(200);
  
}

void TaskAlarm(void *pvParameters) {
  
  Serial.print("Created Allarm Task on core: ");
  Serial.println(xPortGetCoreID());

  while (1)
  {
    /*  SOSPENSIONE ALLARME */
    xSemaphoreTake(mutex_alarm, portMAX_DELAY);

    /*  SEMAFORO SEZIONE CRITICA */
    xSemaphoreTake(mutex_home, portMAX_DELAY);
    home.alarm_sound = ON;
    xSemaphoreGive(mutex_home);

    client.publish(topic_alarm_sound_text, "ALARM ON");
    digitalWrite(ALARM_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(200);
    Serial.println("ALARM ON");
    
    /*  SOSPENSIONE ALLARME */
    xSemaphoreTake(mutex_alarm, portMAX_DELAY);

   /*  SEMAFORO SEZIONE CRITICA */
    xSemaphoreTake(mutex_home, portMAX_DELAY);
    home.alarm_sound = OFF;
    xSemaphoreGive(mutex_home);

    client.publish(topic_alarm_sound_text, "ALARM OFF");
    digitalWrite(BUZZER, LOW);
    digitalWrite(ALARM_LED, LOW);
    Serial.println("ALARM OFF");

  }  
}

void TaskSlave(void *pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 300 / portTICK_PERIOD_MS;
  // Initialise the xLastWakeTime variable with the current time.
  xLastWakeTime = xTaskGetTickCount();
  
  /* ID SLAVE */
  uint32_t id = (uint32_t)pvParameters;
    
  Serial.print("Created Task ");
  Serial.print(id);
  Serial.print(" on core: ");
  Serial.println(xPortGetCoreID());

  char topic_slaves[24]; // Salvo le stringhe dei topic di ogni slave
  snprintf(topic_slaves, 24, "KfZ91%%%%7BM@/Window/%d", id); 
  
  while (1)
  {  
     if(digitalRead(TERMINALS[id]) == HIGH)
      {
      
        xSemaphoreTake(mutex_home, portMAX_DELAY);
        
        home.slave_state[id] = !home.slave_state[id];
        if (home.slave_state[id]) {
          home.open_slaves++;
        } else {
          home.open_slaves--;
        }
        char value[5];
        snprintf(value, sizeof(value), "%d", home.open_slaves);
        client.publish(topic_open_slaves, value);             
        client.publish(topic_slaves, ((home.slave_state[id]) ? "OPEN" : "CLOSED"));
        digitalWrite(LED[id],home.slave_state[id]);
        Serial.print("Window ");
        Serial.print(id);
        Serial.print((home.slave_state[id]) ? " OPEN " : " CLOSED ");

        if (home.slave_state[id] && home.alarm_mode && !home.alarm_sound) {
          xSemaphoreGive(mutex_home);
          xSemaphoreGiveFromISR(mutex_alarm, 0);
        } else {
          xSemaphoreGive(mutex_home);    
        }  
       }

      vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
}

void TaskConnection(void *pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 500 / portTICK_PERIOD_MS;
  // Initialise the xLastWakeTime variable with the current time.
  xLastWakeTime = xTaskGetTickCount();
  
  Serial.print("Created Connection Task on core: ");
  Serial.println(xPortGetCoreID());
  
  /* Connessione Wifi */
  WifiConnection();

  /* Connessione MQTT */
  mqttConnection();
  
  while (1)
  {
    client.loop();
    
    if (!client.connected()) {
      reconnect();
    }
    
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  } 
}

void TaskMain(void *pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 500 / portTICK_PERIOD_MS;
  // Initialise the xLastWakeTime variable with the current time.
  xLastWakeTime = xTaskGetTickCount();
    
  Serial.print("Created Task Main on core: ");
  Serial.println(xPortGetCoreID());

  while (1)
  {  
    if (client.connected() && ACK) {      
      if (strcmp(topic_id, topic_alarm_sound) == 0)   /* Verifica disattivazione suono allarme */
      {
        xSemaphoreTake(mutex_home, portMAX_DELAY);
        if (home.alarm_mode && home.alarm_sound) {
          client.publish(topic_alarm_received, "sound disabled");
          xSemaphoreGive(mutex_home);
          xSemaphoreGiveFromISR(mutex_alarm, 0);
          Serial.println(topic_payload);
        } else if (!home.alarm_mode) {
          client.publish(topic_alarm_received, "Alarm already disabled!");
          xSemaphoreGive(mutex_home);
        } else if (home.alarm_mode && !home.alarm_sound) {
          client.publish(topic_alarm_received, "Sound is off!");
          xSemaphoreGive(mutex_home);
        }
        
      } 
      else if (strcmp(topic_id, topic_alarm_mode_on) == 0)  /* Verifica abilitazione allarme */
      {
        xSemaphoreTake(mutex_home, portMAX_DELAY);
        if (!home.alarm_mode && !home.alarm_sound && home.open_slaves == 0)
        {
          home.alarm_mode = ENABLED;
          client.publish(topic_alarm_received, topic_payload);
          xSemaphoreGive(mutex_home);
          Serial.println(topic_payload);
        } else if (home.open_slaves)
        {    
          client.publish(topic_alarm_received, "Slaves open, can't enable alarm");
          xSemaphoreGive(mutex_home); 
        } else if (home.alarm_mode)
        {
          client.publish(topic_alarm_received, "Alarm already enabled!");
          xSemaphoreGive(mutex_home);
        } else if (home.alarm_sound)
        {
          client.publish(topic_alarm_received, "Alarm sound is ON!");
          xSemaphoreGive(mutex_home);
        }
      } 
      else if (strcmp(topic_id, topic_alarm_mode_off)  == 0)  /* Verifica disabilitazione allarme */
      {
        xSemaphoreTake(mutex_home, portMAX_DELAY);
        home.alarm_mode = DISABLED;
        client.publish(topic_alarm_received, topic_payload);
        xSemaphoreGive(mutex_home);
        Serial.println(topic_payload);
      } 
      else if (strcmp(topic_id, topic_motion_detection_code)  == 0)  /* Verifica disabilitazione Timer allarme tramite Codice */
      {
        Serial.println(topic_payload);
        TickType_t xRemainingTime;
        xRemainingTime = xTimerGetExpiryTime(Timer_code) - xTaskGetTickCount();
        Serial.print("Remaining time to activate alarm: ");
        Serial.println(xRemainingTime);
        xTimerStop(Timer_code, 0);
        Serial.println("Timer stopped");
        client.publish(topic_alarm_received, "Timer stopped");
      } 
      else {
        Serial.println("Message with no topic");
      }
      memset(topic_id, 0, sizeof(topic_id));
      memset(topic_payload, 0, sizeof(topic_payload));
      ACK = 0;
    }

     vTaskDelayUntil( &xLastWakeTime, xFrequency);
  }
  
}

void TaskMovementDetection(void *pvParameters) {

  Serial.print("Created movement detection Task on core: ");
  Serial.println(xPortGetCoreID());

  Ultrasonic ultrasonic(MOV_TRIG, MOV_ECHO);

  xTimerStart(Timer_MovementDetection, 0);
  int val;
   
   while (1)
  {
    xSemaphoreTake(mutex_movement, portMAX_DELAY);
        
    if (!first_read) {
      val = ultrasonic.distanceRead();
      if(val != 0){
        last_distance = val;
        curr_distance = val;
        first_read = true;
      }
    } else {
        last_distance = curr_distance;
        val = ultrasonic.distanceRead(); 
          if(val != 0){
            curr_distance = val;
            // Serial.print("last_distance: ");
            // Serial.print(last_distance);
            // Serial.println("cm");
            // Serial.print("curr_distance: ");
            // Serial.print(curr_distance);
            // Serial.println("cm");
            // Serial.print("Difference: ");
            // Serial.print(curr_distance - last_distance);
            // Serial.println("cm");
            
          }
    
     if ((curr_distance - last_distance) > 5 || (curr_distance - last_distance) < -5 ) {
          Serial.println("MOVEMENT DETECTED");
          xSemaphoreTake(mutex_home, portMAX_DELAY);
          if (!home.alarm_sound)
          {
            xSemaphoreGive(mutex_home);
            xTimerStart(Timer_code, 0);
          } else {
            xSemaphoreGive(mutex_home);
          }
        }    
    }
    
  } 
}

void WifiConnection() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttConnection() {
    if (client.connect(clientid)) {
    Serial.println("Connected to MQTT");
    subscriptions();
  } else {
    Serial.print("Connection to MQTT FAILED - State: ");
    Serial.println(client.state());
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (client.connect(clientid)) {
     Serial.println("Reconnected to MQTT");
     subscriptions();
    }
  }
}

void subscriptions() {
  client.subscribe(topic_alarm_sound);
  client.subscribe(topic_alarm_mode_on);
  client.subscribe(topic_alarm_mode_off); 
  client.subscribe(topic_motion_detection_code);  
}

void callback(char* topic, byte* payload, unsigned int length) {
  int ret;
  if ((int)strlen(topic) > 30) {
    ret = sprintf(topic_id, "%s", "Error");
    if (ret < 0){
      Serial.println("Errore callback");
    }
  }
  else {
    ret = sprintf(topic_id, "%s", topic);
    if (ret < 0){
      Serial.println("Errore callback");
    }
  }

  Serial.print("Message arrived [");
  Serial.print(topic_id);
  Serial.print("] ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    topic_payload[i] = (char)payload[i];
  }
  topic_payload[length] = 0;
  ACK = 1;
  Serial.println();

  // whatever you want for this topic
}

void timer_callback(TimerHandle_t xTimer) {
  if (xTimer == Timer_MovementDetection) 
  {    
    xSemaphoreTake(mutex_home, portMAX_DELAY);
    if (home.alarm_mode && !home.alarm_sound) 
    {
      xSemaphoreGive(mutex_home);
      xSemaphoreGive(mutex_movement);
    } else {
       xSemaphoreGive(mutex_home);
    }
  }
  if (xTimer == Timer_code) 
  {
      xSemaphoreGiveFromISR(mutex_alarm, 0);
  } 
}

void loop() {
  // put your main code here, to run repeatedly:
}

    /* DEBUG utilizzato per dimensione STACK degli task */

    // Serial.print(pcTaskGetTaskName(NULL));
    // Serial.print(" uxTaskGetStackHighWaterMark = ")
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));
