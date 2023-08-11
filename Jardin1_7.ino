// **********************************************************************************
// **** Garden Sprinkler
// **********************************************************************************
// *** 2022/07/20 - V0.0 - Module de base
// *** 2022/07/21 - V1.0 - Module avec 2 vannes
// *** 2022/07/22 - V1.1 - Ajout gestion des jours
// *** 2022/07/24 - V1.2 - Correction bugs gestion des jours
// *** 2022/07/28 - V1.3 - Correction interface web
// *** 2023/06/26 - V1.4 - Modification de l'affichage OLED avec ajout du "robinet qui coule"
// *** 2023/07/15 - V1.5 - Ajout gestion de l'hygrométrie
// *** 2023/07/16 - V1.6 - Nettoyage du code pour tout reposer sur la fonction de collecte des donnée en flash - contenuFlash()
// *** 2023/07/18 - V1.7 - Ajout du label sur les programme pour identifier plus facilement l'affectation des vannes 
// ***********************************************************************************


#define DEBUGcontenuFlash 
#define DebugProgIrri


#define versionFile 1.7


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include <Arduino_JSON.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Jardin1HTML.h"
#include <EEPROM.h>
#include <ArduinoOTA.h>


// *** Tuto ecran SSD1306 *** https://passionelectronique.fr/ecran-oled-i2c-arduino/
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
//========= Police de caractère : AlarmClock (https://www.dafont.com/fr/alarm-clock.font)
#include "alarm_clock6pt7b.h"  // Normal, en 6 pts

#include "Pixel_LCD_76pt7b.h"
#include "Pixel_LCD_710pt7b.h"
#include "Pixel_LCD_715pt7b.h"

//========= Police de caractère : FreeMono (fournie avec librairie Adafruit GFX)
#include <Fonts/FreeMono9pt7b.h>  // Normal (en 9, 12, 18, et 24 pts)


#define STASSID "Freebox-manu"
#define STAPSK  "nugor5-subduxi-factitando37-indemne"

//#define STASSID "MaisonPaulRomy"
//#define STAPSK  "Maisonpaul&romy"

//#define STASSID "CNuma"
//#define STAPSK "r257-ZGiz-JNC6-zhq9"

// Avec intervalle de mise à jour de 60 sec ...
// ... et offset en secondes cad decalage horaire, pour GMT+1 mettre 3600, GMT +8 mettre 28800, etc.
// ... et le serveur NTP utilise est : europe.pool.ntp.org
//Creation objet WIFI UDP puis du client NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);
//NTPClient timeClient(ntpUDP, "192.168.45.173", 7200, 60000);


int groundWet;

float sensorReadingsArr[4];
//const char* serverName = "http://192.168.1.150/meteo/ESPGetTempMinMax.php?TempOrigine=JardinHum1";

HTTPClient http;
WiFiClient client;


#define timeOutCnx 50000
#define timeOutBlink 500



int nbrProg;
int posVarFlash;

#define nombreProgrammeDispo 5
#define labelSize 10
#define tailleProgramme 7 + labelSize    // *** 17 octets par programme - Etat/H/M/Durée/Jours(binaire 8b)/vanne/hygrometrie/Label 10 caracteres  -
#define EEPROM_SIZE nombreProgrammeDispo * tailleProgramme  
int varFlash[tailleProgramme];
            // *** Chaque programme comporte 7 octets:
            // ***    0 - Etat du programme - 0/Non disponible, 1/Operationnel, 2/Operationnel Eco, 255/Non init.
            // ***    1 - Heure
            // ***    2 - Minute
            // ***    3 - Durée
            // ***    4 - Jour en mode binaire - Lundi b0, mardi b1, ...
            // ***    5 - Numéro de vanne
            // ***    6 - Taux hygrometrie du sol pour le déclenchement du programme - nécessite la sonde de mesure ! -1 -> pas de sonde !
            // ***    7-17 - 10 caracteres pour mettre un label sur le programme 

int progVanneStatus[nombreProgrammeDispo];

int progVanneHDebut[nombreProgrammeDispo];
int progVanneMDebut[nombreProgrammeDispo];
int progVanneHFin[nombreProgrammeDispo];
int progVanneMFin[nombreProgrammeDispo];
int progVanneDuree[nombreProgrammeDispo];

int progVanneJours[nombreProgrammeDispo];
int progVanneRef[nombreProgrammeDispo];

int progVanneHygrometrie[nombreProgrammeDispo];


char progVanneLabel[nombreProgrammeDispo][labelSize];


#define pinV0 13
#define pinV1 14
byte V0 = 0, V1 = 0;


String HeureNTP;
String couranteHeure;
String couranteMinute;

const char* ssid = STASSID;
const char* password = STAPSK;

// Set web server port number to 80
ESP8266WebServer server(80);

// Variable to store the HTTP request
String header;


unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;  // Define timeout time in milliseconds for webPage activities

// Variables de travail
unsigned long epoch = 0;

int nujour = 0;  //numero jour de la semaine avec 0 pour dimanche
int nujourBin;
String jour = "mon jour";  // dimanche, lundi, etc.
String heure = "mon heure ..";

String jourSemaine[] = { "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche" };
String jourSemaineMins[] = { "L", "M", "M", "J", "V", "S", "D" };

String statusProgramme[] = { "Non utilisé", "Operationnel", "Operationnel ECO" };

byte ledStatus;



#define nombreDePixelsEnLargeur 128  // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define nombreDePixelsEnHauteur 64   // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define brocheResetOLED -1           // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define adresseI2CecranOLED 0x3C     // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)


#define xref 88
#define yref 25
int bulletPosH;
int bulletPosV;

int i,j;
Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, -1);

long afficheurTimer;

#define BPafficheur 12
long BPpushedMillis;
long BPpushedMillisPage;
byte BPpushed;
byte BPlastState;

int afficheurPage;




void ICACHE_RAM_ATTR BPMode () {
  BPpushedMillis = millis();
}




void setup() {

  Serial.begin(115200);
  Serial.println("Booting");

  ArduinoOTA.setHostname("jardin-1-7");
  ArduinoOTA.setPassword("123456");
  ArduinoOTA.begin();

  Serial.print("Pin2 output");
  pinMode(2, OUTPUT);

  pinMode(pinV0, OUTPUT);
  pinMode(pinV1, OUTPUT);

  pinMode(BPafficheur, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BPafficheur), BPMode, FALLING);

  Serial.print("Init SSD1306");
  // Initialisation de l'écran OLED
  if (!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED)) {
    Serial.println(F("Erreur de communication avec le chipset SSD1306… arrêt du programme."));
    digitalWrite(2, HIGH);
    while (1) {
      digitalWrite(2, HIGH);
        delay(1500);
        digitalWrite(2, LOW);
        delay(550);
    }
      ;  // Arrêt du programme (boucle infinie)

  } else {
    Serial.println(F("Initialisation du contrôleur SSD1306 réussi."));
    ecranOLED.clearDisplay();

    ecranOLED.setTextSize(1);    // Taille des caractères (1:1, puis 2:1, puis 3:1)
    ecranOLED.setCursor(15, 0);  // Déplacement du curseur en position (0,0), c'est à dire dans l'angle supérieur gauche
    ecranOLED.setTextColor(SSD1306_WHITE);
    ecranOLED.print("cNuma Bonjour ");

    ecranOLED.setCursor(40, 55);  // Déplacement du curseur en position (0,0), c'est à dire dans l'angle supérieur gauche
    ecranOLED.setTextColor(SSD1306_WHITE);
    ecranOLED.print("2023/06 V" + String(versionFile));

    ecranOLED.display();  // Transfert le buffer à l'écran
  }


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  previousTime = millis();

  ecranOLED.setCursor(0, 20);
  ecranOLED.print("IP: ");
  ecranOLED.display();

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, HIGH);
    delay(150);
    Serial.print(".");

    ecranOLED.print(".");
    ecranOLED.display();
    

    digitalWrite(2, LOW);
    delay(150);

    if (millis() > (previousTime + timeOutCnx)) {
      Serial.print(F("####### ===> Too long to connect, >>> REBOOT <<< ! "));
      ecranOLED.print("Too long - reboot !");
      ecranOLED.display();
      
      digitalWrite(2, LOW);
      delay(1500);
      for (int i = 0; i <= 10; i++) {
        digitalWrite(2, HIGH);
        delay(50);
        digitalWrite(2, LOW);
        delay(50);
      }
      ESP.restart();
    }
  }

  ecranOLED.print(" Ok !");
  ecranOLED.display();

  Serial.println(F("#######################################"));
  Serial.println(F("WiFi connected."));
  Serial.print(F("### IP address: "));
  Serial.println(WiFi.localIP());

  // ***************************************************************************************************************
  // **************************************** EEPROM definition ****************************************************
  ecranOLED.setCursor(0, 30);
  ecranOLED.print("EEPROM Def ...");
  ecranOLED.display();

  EEPROM.begin(EEPROM_SIZE);
  delay(500);


  // ***************************************************************************************************************
  // **************************************** Server page definition ***********************************************
  
  ecranOLED.setCursor(0, 40);
  ecranOLED.print("Server Start !");
  ecranOLED.display();

  server.on("/", handleRoot);
  server.on("/heureESP", heureESP);
  server.on("/progirri", progIrri);
  server.on("/progslot", progSlot);
  server.on("/majProg", majProg);
  server.on("/about", affichePageAboutManu);
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");


  // Demarrage client NTP
  Serial.println(F("#######################################"));
  Serial.println("Demarrage client NTP ! ");
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(7200);
  
  groundWet = humiditeSol().toInt();

  previousTime = millis();
}

void loop() {

  ArduinoOTA.handle();
  
  server.handleClient();

  // *** Gestion du bouton poussoir de bascule entre les pages ***
  if ((millis() - BPpushedMillis) > 100) {
    if (digitalRead(BPafficheur) == LOW ) {
      Serial.println(F("#### - BP appuyé - afficheurPage:") + String(afficheurPage));
      afficheurTimer = millis();

      if ((millis() - BPpushedMillisPage) > 200) {
        BPpushedMillisPage = millis();
        if (afficheurPage < nombreProgrammeDispo + 1) {
          afficheurPage = afficheurPage + 1;
        } else {
          afficheurPage = 0;
        }
      }

    } 
  }


  if (millis() < (afficheurTimer + 15000)) {
    
    ecranOLED.clearDisplay();
    //contenuFlash();

    if (afficheurPage == 0) {
        ecranOLED.setFont(&Pixel_LCD_710pt7b);  // Sélection de la police "Alarm Clock"
        ecranOLED.setTextSize(1);               // Taille des caractères (1:1, puis 2:1, puis 3:1)
        ecranOLED.setCursor(5, 32);             // Déplacement du curseur en position (X,Y)
        ecranOLED.setTextColor(SSD1306_WHITE);
        ecranOLED.print(HeureNTP);

        ecranOLED.setFont(&FreeMono9pt7b);  // Sélection de la police "Free Mono"
        ecranOLED.setCursor(3,  47);
        ecranOLED.println("V0:" + String(V0));
        ecranOLED.setCursor(85, 47);
        ecranOLED.println("V1:" + String(V1));  // …et affichage du nom de cette police

        Serial.println("Affiche groundWet");
        ecranOLED.setCursor(0, 10);
        ecranOLED.println("Hum. sol " + humiditeSol());  // …et affichage du nom de cette police

/*
        if (groundWet > 60) {
          Serial.println("Trop humide - sup a 60, Vanne 0 fermée !");
          ecranOLED.setCursor(0, 60);  // Déplacement du curseur en position (0,0), c'est à dire dans l'angle supérieur gauche
          ecranOLED.setFont(NULL);   
          ecranOLED.setTextSize(1); 
          ecranOLED.setTextColor(SSD1306_WHITE);
          ecranOLED.print("Sol trop humide >60 !");
        }
*/
        


    } else {
      if ((afficheurPage-1) < nombreProgrammeDispo) {
        affichePageProgramme((afficheurPage-1));
      } else {
        affichePageAboutManu();
      }           
    }
  } else {
    afficheurPage = -1;   
    ecranOLED.clearDisplay();
    ecranOLED.display();
  }

  ecranOLED.display();  // Transfert le buffer à l'écran
  

  if (millis() > (previousTime + timeOutBlink)) {
    previousTime = millis();

    //Serial.println(F("RSSI: ") + String(WiFi.RSSI()) + "dBm - ");

    if (ledStatus == 0) {

      digitalWrite(2, !digitalRead(2));

      contenuFlash();

      // *** reset paramètres vanne
      V0 = 0;
      V1 = 0;

      // Recup heure puis affichage
      timeClient.update();
      epoch = timeClient.getEpochTime();  // Heure Unix
      nujour = timeClient.getDay();       // jour de la semaine
      nujourBin = 1;
      for (j = 1; j < nujour; j++) {
        nujourBin = nujourBin << 1;
      }
      heure = timeClient.getFormattedTime();  // heure

      // Envoi des donnees recuperees sur la liaison seriE
      //Serial.print("Temps UNIX : ");
      //Serial.print(epoch);
      //Serial.print(" - jour : ");
      //Serial.print(nujour);
      //Serial.print(" - heure : ");
      HeureNTP = timeClient.getFormattedTime();
      //Serial.println(HeureNTP);
      couranteHeure = HeureNTP.substring(0, HeureNTP.indexOf(":"));
      couranteMinute = HeureNTP.substring(3, HeureNTP.indexOf(":", 3));
      ;

      if (couranteMinute == "00") {
        groundWet = humiditeSol().toInt();
      }

      Serial.println("########################## Affichage etat des programmes ####");

      for (nbrProg = 0; nbrProg <= nombreProgrammeDispo - 1; nbrProg++) {

        /*Serial.println("## Prog: " + String(nbrProg));
        Serial.print("progVanneJours[nbrProg] :");
        Serial.print(progVanneJours[nbrProg], BIN);
        Serial.print(" - nujourBin :");
        Serial.print(nujourBin, BIN);
        Serial.println("(progVanneJours[nbrProg] & (nujour-1)) :" + String((progVanneJours[nbrProg] & (nujourBin))));
        
        // ************************************** Gestion de la vanne 0 selon les slots définis ******************************************

        Serial.println(" groundWet:" + String(groundWet));
        Serial.println(" progVanneHygrometrie:" + String(progVanneHygrometrie[nbrProg]));
        */

        if ((groundWet < progVanneHygrometrie[nbrProg]) && ((progVanneJours[nbrProg] & (nujourBin)) != 0)) {
          if (progVanneRef[nbrProg] == 0) {
            if ((couranteHeure.toInt() >= progVanneHDebut[nbrProg]) && (couranteHeure.toInt() <= progVanneHFin[nbrProg])) {
              Serial.println(" -- V0 --- H OK !");
              if ((couranteMinute.toInt() >= progVanneMDebut[nbrProg]) && (couranteMinute.toInt() < progVanneMFin[nbrProg])) {
                Serial.println("Vanne 0 active");
                V0 = 1;
              } else {
               /* Serial.println(" -- V0 --- Min pas bon - vanne 0 fermée !");

                Serial.println(" -- V0 ---couranteMinute.toInt(): " + String(couranteMinute.toInt()));
                Serial.println(" -- V0 ---progVanneMDebut[nbrProg]: " + String(progVanneMDebut[nbrProg]));
                Serial.println(" if1: " + String(couranteMinute.toInt() >= progVanneMDebut[nbrProg]));
                
                
                Serial.println(" -- V0 ---couranteMinute.toInt(): " + String(couranteMinute.toInt()));
                Serial.println(" -- V0 ---progVanneMFin[nbrProg]): " + String(progVanneMFin[nbrProg]));
                Serial.println(" if2: " + String(couranteMinute.toInt() < progVanneMFin[nbrProg]));
                */

                //V0 = 0;
              }
            } else {
              Serial.println("H pas bon, Vanne 0 fermée !");
              //V0 = 0;
            }
          }  // *** IF pour Vanne 0 ***
        } else {
          Serial.println(" ## J pas bon - V0 fermée");

          if (groundWet < progVanneHygrometrie[nbrProg]) {
            Serial.println("#### Hygrométrie sol trop importante - vanne reste fermée ");
          }
          
        }


        // ************************************** Gestion de la vanne 1 selon les slots définis ******************************************
        if ((groundWet < progVanneHygrometrie[nbrProg]) && (progVanneRef[nbrProg] == 1)) {
          if ((couranteHeure.toInt() >= progVanneHDebut[nbrProg]) && (couranteHeure.toInt() <= progVanneHFin[nbrProg])) {
            Serial.println(" -- V1 H OK !");
            if ((couranteMinute.toInt() >= progVanneMDebut[nbrProg]) && (couranteMinute.toInt() < progVanneMFin[nbrProg])) {
              Serial.println("Vanne 1 active");
              V1 = 1;
            } else {
              Serial.println("Min pas bon - vanne 1 fermée !");
              //V1 = 0;
            }
          } else {
            Serial.println("H pas bon, Vanne 1 fermée !");

          
          }
        }  // ** IF pour vanne 1




      }  // *** FOR      
    }

    if (V0==1) {
      digitalWrite(pinV0, HIGH);
      Serial.println("V0 relais HIGH");
    } else {
      digitalWrite(pinV0, LOW);
      Serial.println("V0 relais LOW");
    }

    
    if (V1==1) {
      digitalWrite(pinV1, HIGH);
      Serial.println("V1 relais HIGH");
    } else {
      digitalWrite(pinV1, LOW);
      Serial.println("V1 relais LOW");
    }


  } 

}


String humiditeSol(){

  String groundWet = "-1";

  if(WiFi.status() == WL_CONNECTED){
        //sensorReadings = http.httpGETRequest(serverName);
        http.begin(client,"http://192.168.1.150/meteo/ESPGetTempMinMax.php?TempOrigine=JardinHum1");

        int httpCode = http.GET();                                  //Send the request
 
        if (httpCode > 0) { //Check the returning code
    
          String payload = http.getString();   //Get the request response payload
          Serial.println(payload);             //Print the response payload

          JSONVar myObject = JSON.parse(payload);

          Serial.print("JSON object = ");
          Serial.println(myObject);

          JSONVar keys = myObject.keys();
/*
        for (int i = 0; i < keys.length(); i++) {
          JSONVar value = myObject[keys[i]];
          Serial.print(keys[i]);
          Serial.print(" = ");
          Serial.println(value);
          sensorReadingsArr[i] = double(value);
        }
*/        
        Serial.print("Time last put to DB = ");
        Serial.println(myObject[keys[0]]);
        
        
        Serial.print("valeur de GroundWet = ");
        groundWet = String(myObject[keys[1]]);
        //groundWet.remove(0,1);
        //groundWet.remove(groundWet.length()-1);
        
        Serial.print(groundWet);

        Serial.print("%");
        
        }
    
        http.end();   //Close connection
        
      } else {
        Serial.println("####### Pas connecté - pas de GET");
        groundWet = "-2";
      }

      return groundWet;
}




void affichePageAboutManu() {

    ecranOLED.clearDisplay();

    ecranOLED.setFont(NULL);   
    ecranOLED.setTextSize(1);    // Taille des caractères (1:1, puis 2:1, puis 3:1)
    ecranOLED.setCursor(5, 0);  // Déplacement du curseur en position (0,0), c'est à dire dans l'angle supérieur gauche
    ecranOLED.setTextColor(SSD1306_WHITE);
    ecranOLED.print("#cNuma 2023/06 V" + String(versionFile));

    ecranOLED.setCursor(0, 20);
    ecranOLED.print("@ip: ");
    ecranOLED.println(WiFi.localIP());


    ecranOLED.display();  // Transfert le buffer à l'écran

}

void affichePageProgramme (int prog){
  ecranOLED.drawRoundRect(0, 0, 127, 16, 5, WHITE);
  ecranOLED.drawRoundRect(0, 17, 85, 63-16, 5, WHITE);
  
  ecranOLED.setTextColor(SSD1306_WHITE);
  ecranOLED.setFont(NULL);    
  ecranOLED.setTextSize(1);
  
  ecranOLED.setCursor(4, 4);
  //ecranOLED.print("P0 - Actif ECO");

  switch (progVanneStatus[prog]) {
    case 0:
      ecranOLED.print("P" + String(prog) + " Arret");
      break;
    case 1:
      ecranOLED.print("P" + String(prog) + " Actif");
      break;
    case 2:
      ecranOLED.print("P" + String(prog) + " Actif ECO");
      break;
    default:
      ecranOLED.print("P" + String(prog) + " Flash Err.");
      break;
  }

  if ((progVanneStatus[prog] == 1) || (progVanneStatus[prog] == 2)) {
    ecranOLED.drawLine(0, 32, 84, 32, WHITE);
    ecranOLED.drawLine(0, 47, 84, 47, WHITE);

    ecranOLED.setCursor(4, 21);
    ecranOLED.print(String(progVanneHDebut[prog]) + ":" + String(progVanneMDebut[prog]) + "  " + String(progVanneDuree[prog]) + " min");

    // ** robinet horizontal
    ecranOLED.fillRoundRect(xref, yref, 27, 7, 2, WHITE);
    // ** robinet rotation du bout
    ecranOLED.fillCircleHelper(xref+24, yref+7, 7, 1, 0, WHITE);
    // ** robinet descente
    ecranOLED.fillRect(xref+24, yref+7, 8, 8,  WHITE);
    // ** robinet bouton de commande
    ecranOLED.fillRect(xref+13,yref-7,7,7,WHITE);
    ecranOLED.fillRect(xref+10,yref-5,13,2,WHITE);

    // *** jours de la semaine et séparation
    for (int k=1;k<=6;k++) {
      ecranOLED.drawFastVLine(12*k, 32, 32, WHITE);
      ecranOLED.setCursor(4+(k-1)*12, 37);
      ecranOLED.print(jourSemaineMins[k-1]);
    }
    // *** affiche juste Dimanche
    ecranOLED.setCursor(4+6*12, 37);
    ecranOLED.print(jourSemaineMins[6]);

    int masquejour = 1;
    for (int j = 0; j <= 6; j++) {
      if ((masquejour & progVanneJours[prog]) == masquejour) {                
        ecranOLED.fillTriangle(j*12+6, 50, j*12+3, 57, j*12+9, 57, WHITE);
        ecranOLED.fillCircle(j*12+6, 57, 3, WHITE);
        ecranOLED.fillCircle(j*12+6, 57, 2, BLACK);
      }
      masquejour = masquejour << 1;
    }  

    ecranOLED.setCursor(92, 43);
    ecranOLED.print("V " + String(progVanneRef[prog]));

    ecranOLED.setCursor(88, 55);
    switch (progVanneRef[prog]) {
      case 0:
        if (V0 == 0) {
          ecranOLED.print("Fermee");      
        } else {
          ecranOLED.print("Ouvert");
          robinetCoule();
        }
      
      break;
      
      case 1:
        if (V1 == 0) {
          ecranOLED.print("Fermee");      
        } else {
          ecranOLED.print("Ouvert");
          robinetCoule();
        }
      
      break;
    }

  } else {
    ecranOLED.drawRoundRect(0, 17, 85, 63-16, 5, BLACK);
  }
}

void robinetCoule(){
    if(bulletPosH<=20) {
    ecranOLED.fillCircle(xref+3+bulletPosH   , yref+3, 2, WHITE);
    ecranOLED.fillCircle(xref+3+bulletPosH+1 , yref+3, 2, BLACK);
    bulletPosH++;
  } else {
    ecranOLED.fillCircle(xref+3+bulletPosH   , yref+3, 2, WHITE);
    if(bulletPosV<=15) {
      ecranOLED.fillCircle(xref+6+bulletPosH  , yref+3+bulletPosV, 2, WHITE);
      ecranOLED.fillCircle(xref+6+bulletPosH  , yref+3+bulletPosV+1, 2, BLACK);
      ecranOLED.fillRect(xref+24, yref+7+8, 8, 4,  BLACK);
      bulletPosV++;
    } else {
      bulletPosH=0;
      bulletPosV=0;
    }
  }
}


void handleRoot() {
  Serial.print("Root page");
  //String page = "<HTML><BODY><H1>cNuma Jardin</H1></BODY></HTML>";
  //page += "<HR><H2>planning page</H2>";
  String s = webpage;
  server.send(200, "text/html", s);  // Send HTTP status 200 (Ok) and send some text to the browser/client
}


void heureESP() {
  Serial.print("MAJ heure page web");

  String s = timeClient.getFormattedTime();

  s += "|";
  if (V0 == 0) {
    s += "<span style='background-color: #FF0000;'>V0<BR>Vanne Fermée</span>";
  } else {
    s += "<span style='background-color: #00FF00;'>V0<BR>Vanne Ouverte</span>";
  }

  s += "|";
  if (V1 == 0) {
    s += "<span style='background-color: #FF0000;'>V1<BR>Vanne Fermée</span>";
  } else {
    s += "<span style='background-color: #00FF00;'>V1<BR>Vanne Ouverte</span>";
  }

  server.send(200, "text/html", s);
}

// *** MAJ de la flash avec les nouvelles données de programmation ***
void majProg() {

  int programme = (server.arg("programme")).toInt();

  // => Etat/H/M/Durée/Jours(binaire 8b)/vanne/hygrometrie/label-Programme/10car

  // *** ETAT du programme ***
  Serial.println("### Ecriture de l'ETAT: " + server.arg("inputStatusProg"));
  EEPROM.write(programme * tailleProgramme, server.arg("inputStatusProg").toInt());

  // *** HH du programme ***
  Serial.println("### Ecriture de HH: " + server.arg("inputHH"));
  EEPROM.write(programme * tailleProgramme + 1, server.arg("inputHH").toInt());

  // *** MM du programme ***
  Serial.println("### Ecriture de MM: " + server.arg("inputMM"));
  EEPROM.write(programme * tailleProgramme + 2, server.arg("inputMM").toInt());

  // *** DUREE du programme ***
  Serial.println("### Ecriture de DUREE: " + server.arg("inputDelay"));
  EEPROM.write(programme * tailleProgramme + 3, server.arg("inputDelay").toInt());

  // *** JOUR du programme ***
  int masque = 1;
  int joursActif = 0;
  for (j = 0; j <= 6; j++) {
    Serial.print("###  jourSemaine[j]: " + jourSemaine[j]);
    Serial.println("###  server.arg(jourSemaine[j]: " + server.arg(jourSemaine[j]));
    if (server.arg(jourSemaine[j]) == "true") {
      joursActif = joursActif + masque;
      Serial.print("### Checked !!! joursActif: ");
      Serial.println(joursActif, BIN);
    }
    masque = masque << 1;
    Serial.print("### Nouvelle valeur du masque: ");
    Serial.println(masque, BIN);
  }

  Serial.print("#### jours joursActif FINAL: ");
  Serial.println(joursActif, BIN);

  Serial.println("### Ecriture de JOURS: " + joursActif);
  EEPROM.write(programme * tailleProgramme + 4, joursActif);

  // *** VANNE affectée au programme *** octet 5
  Serial.println("### Ecriture de VANNE: " + server.arg("inputVanne"));
  EEPROM.write(programme * tailleProgramme + 5, server.arg("inputVanne").toInt());

  // *** Hygrometrie de déclenchement du programme *** octet 6
  Serial.println("### Ecriture de HYGROMETRIE: ##" + server.arg("inputHygrometrie") + "##");
  EEPROM.write(programme * tailleProgramme + 6, server.arg("inputHygrometrie").toInt());

  // *** Label pour le programme *** octets 7 à 17
  Serial.println("### Ecriture du label du programme: ##" + server.arg("inputLabel") + "##");

  String inputLabel = server.arg("inputLabel");
  Serial.println("inputLabel" + inputLabel);
  for (int i=0; i <= (labelSize - 1); i++) {
    Serial.println("Ecriture de:" + inputLabel[i]);
    EEPROM.write(programme * tailleProgramme + 7 + i, inputLabel[i]);
  }
  

  EEPROM.commit();
}


// ********************** Construction de la MODAL pour modification des paramètres de programmation **********************************************
void progSlot() {

  Serial.println(F("####### Recup etat de la programmation en FLASH"));
  contenuFlash();


  String s;


  int nbrProg = server.arg("programme").toInt();

  s = "<div class=\"row inline-block\" >";
  s += "<form action=updateProgIrri method=\"get\" >";
  s += "<input type=\"text\" id=\"programme\" value=\"" + server.arg("programme") + "\" hidden>";
  s += "</div>";

  // **** Etat du programme *****
  s += "<div class=\"form-group row\" >";
  s += "<label for=\"inputStatusProg\" class=\"col-sm-4 col-form-label\">Etat du programme</label>";
  s += "<div class=\"col-sm-8 \">";

  s += "<select class=\"custom-select\" id=\"inputStatusProg\">";
  for (int i = 0; i <= 2; i++) {
    s += "<option ";
    if (progVanneStatus[nbrProg] == i) {
      s += "selected";
    }
    s += " value=\"" + String(i) + "\">" + statusProgramme[i] + "</option>";
  }
  s += "</select>";


  s += "</div>";
  s += "</div>";


  // *** Label associé au programme ***
  s += "<div class=\"form-group row\" >";
  s += "<label for=\"inputLabel\" class=\"col-sm-4 col-form-label\">Label pour le programme</label>";
  s += "<div class=\"col-sm-8\"><input type=\"text\" size=10 class=\"form-control\" id=\"inputLabel\" placeholder=\"Label pour le programme\" value=\"";
  s += progVanneLabel[nbrProg];
  s += "\" >";
  s += "<small class=\"form-text text-muted\">10 caractères Max !</small>";
  s += "</div></div>";

  // **** Heure de début du programme *****
  s += "<div class=\"form-group row\" >";
  s += "<label for=\"inputHH\" class=\"col-sm-4 col-form-label\">Heure début</label>";
  s += "<div class=\"col-sm-2\"><input type=\"number\" class=\"form-control\" id=\"inputHH\" placeholder=\"HH\" value=\"" + String(progVanneHDebut[nbrProg]) + "\" min=\"0\" max=\"23\"></div>";
  s += "<div class=\"col-sm-2\"><input type=\"number\" class=\"form-control\" id=\"inputMM\" placeholder=\"MM\" value=\"" + String(progVanneMDebut[nbrProg]) + "\" min=\"0\" max=\"59\"></div>";
  s += "</div>";

  // **** Durée du programme *****
  s += "<div class=\"form-group row\" >";
  s += "<label for=\"inputDelay\" class=\"col-sm-4 col-form-label\">Durée</label>";
  s += "<div class=\"col-sm-4\"><input type=\"number\" class=\"form-control\" id=\"inputDelay\" placeholder=\"Max 59 min\" value=\"" + String(progVanneDuree[nbrProg]) + "\" min=\"0\" max=\"59\"></div>";
  s += "</div>";

  // **** Jour d'activité du programme *****
  s += "<div class=\"form-group row\" >";
  s += "<div class=\"col-sm-4 col-form-label\">Jour(s)</div>";
  s += "<div class=\"col-sm-4\">";

  int masque = 1;

  for (int j = 0; j <= 6; j++) {
    s += "<div class=\"form-check\">";
    s += "<input type=\"checkbox\" class=\"form-check-input\" id=\"" + jourSemaine[j] + "\"";

    if ((progVanneJours[nbrProg] & masque) == masque) {
      s += " checked ";
    }

    s += " value=\"" + jourSemaine[j] + "\">";
    s += "<label for=\"" + jourSemaine[j] + "\" class=\"col-sm-4 col-form-labelform-check-label\">";

    s += jourSemaine[j] + "</label>";

    masque = masque << 1;

    s += "</div>";
  }
  s += "</div>";
  s += "</div>";

  // **** Selection de la vanne *****
  s += "<div class=\"form-group row\" >";
  s += "<label for=\"inputVanne\" class=\"col-sm-4 col-form-label\">Vanne</label>";
  s += "<div class=\"col-sm-4\"><input type=\"number\" class=\"form-control\" id=\"inputVanne\" placeholder=\"0 ou 1\" value=\"" + String(progVanneRef[nbrProg]) + "\" min=\"0\" max=\"1\">";
  s += "<small class=\"form-text text-muted\">0 ou 1</small>";
  s += "</div>";
  s += "</div>";


  // **** Détermination du taux d'hygrometrie minimale en dessous duquel le programme se déclenche *****
  s += "<div class=\"form-group row\" >";
  s += "<label for=\"inputHygrometrie\" class=\"col-sm-4 col-form-label\">Hygrometrie de déclenchement</label>";
  s += "<div class=\"col-sm-4\"><input type=\"number\" class=\"form-control\" id=\"inputHygrometrie\" placeholder=\"-1, non utilisé, 0 à 100%\" value=\"" + String(progVanneHygrometrie[nbrProg]) + "\" min=\"-1\" max=\"100\">";
  s += "<small class=\"form-text text-muted\">0% à 100%</small>";
  s += "</div>";
  s += "</div>";

  s += "</form>";

  server.send(200, "text/html", s);
}

void contenuFlash() {

  #ifdef DEBUGcontenuFlash
    Serial.println("################################################################# contenuFlash ###");
  #endif

  // *** Recup contenue de la flash - pour le programme considéré ***
  for (nbrProg = 0; nbrProg <= nombreProgrammeDispo - 1; nbrProg++) {
    for (posVarFlash = 0; posVarFlash <= tailleProgramme - 1 ; posVarFlash++) {
      varFlash[posVarFlash] = EEPROM.read(nbrProg * tailleProgramme + posVarFlash);
    }

    //Serial.println("Maj progVanneStatus" + String(nbrProg) + varFlash[0]);
    progVanneStatus[nbrProg] = varFlash[0];

    //Serial.println("Maj progVanneHDebut");
    progVanneHDebut[nbrProg] = varFlash[1];

    //Serial.println("Maj progVanneMDebut");
    progVanneMDebut[nbrProg] = varFlash[2];

    //Serial.println("Maj progVanneHFin et progVanneMFin");
    if ((varFlash[2] + varFlash[3]) <= 59) {
      progVanneHFin[nbrProg] = varFlash[1];
      progVanneMFin[nbrProg] = varFlash[2] + varFlash[3];
    } else {
      if (varFlash[1] + 1 <= 23) {
        progVanneHFin[nbrProg] = varFlash[1] + 1;
      } else {
        progVanneHFin[nbrProg] = 0;
      }
      progVanneMFin[nbrProg] = varFlash[3] - (60 - varFlash[2]);
    }

    //Serial.println("Maj progVanneDuree");
    progVanneDuree[nbrProg] = varFlash[3];

    //Serial.println("Maj progVanneJours");
    progVanneJours[nbrProg] = varFlash[4];

    //Serial.println("Maj progVanneRef");
    progVanneRef[nbrProg] = varFlash[5];

    //Serial.println("Maj progVanneHygrometrie" + String(nbrProg) + varFlash[6]);
    progVanneHygrometrie[nbrProg] = varFlash[6];
    #ifdef DEBUGcontenuFlash
      Serial.println("### DEBUG # contenuFlash() - varFlash["+ String(nbrProg) + "]: " + String(varFlash[6]));
      Serial.println("### DEBUG # contenuFlash() - progVanneHygrometrie["+ String(nbrProg) + "]: " + String(progVanneHygrometrie[nbrProg]));
    #endif

    //Serial.println("Maj progVanneLabel" + String(nbrProg) + varFlash[6]);
    strcpy(progVanneLabel[nbrProg], "          ");
    for (int i=0; i<=(labelSize - 1); i++) {
      progVanneLabel[nbrProg][i] = char(varFlash[7+i]);
    }

  }
}


// ******************************* Affiche le tableau d'irrigation au chargement de la page HTML ***********************************************
void progIrri() {

  #ifdef DebugProgIrri
    Serial.println(F("####################################################################"));
    Serial.println(F("### DEBUG # progIrri() - Construction page web des slots définis ###"));
  #endif

  String s;

  s = "<div class=\"row inline-block\" >";
    s += "<div class=\"row\" >";
      s += "<div class=\"col-sm-12\"><H2>Humidité du sol: <B>" + String(groundWet) + "%</B></H2>";
/*      if (groundWet > 60 ) {
        s += "<span style=\"background-color:#dd0000;\">Sol trop humide, pas d'arrosage de prévu ...";
      } else {
        s += "<P><span style=\"background-color:#00ff00;\">Humidité du sol en dessous de la limite de 60%, arrosage lors des prochains slots horaires programmés !<P>";
      }
*/      s += "</div>";
    s += "</div>";
  s += "</div>";


  //s = "<form action=updateProgIrri method=\"get\" ><table class=\"table\"><TR><TH>Prog<TH>Status<TH>H:M debut<TH>Durée<TH>Jour<TH>Vanne";
  s += "<div class=\"row inline-block\" >";

  //s = "<form action=updateProgIrri method=\"get\" >";
  s += "<div class=\"row\" >";

  s += "<div class=\"col-sm\">Prog</div>";
  s += "<div class=\"col-sm\">Status</div>";
  s += "<div class=\"col-sm\">Hygrometrie déclenchement</div>";
  s += "<div class=\"col-sm\">H:M debut</div>";
  s += "<div class=\"col-sm\">Durée</div>";
  s += "<div class=\"col-sm\">Jour</div>";
  s += "<div class=\"col-sm\">Vanne</div>";

  s += "</div>";


  // *** Lecture de la FLASH - paramètres de programmes d'arrosage
  // *** Chaque programme comporte 6 octets:
  // ***    0 - Etat du programme - 0/Non disponible, 1/Operationnel, 2/Operationnel Eco, 255/Non init.
  // ***    1 - Heure
  // ***    2 - Minute
  // ***    3 - Durée
  // ***    4 - Jour en mode binaire - Lundi b0, mardi b1, ...
  // ***    5 - Numéro de vanne
  // ***    6 - Taux hygrométrie de déclenchement du programme

  #ifdef DebugProgIrri
    Serial.println(F("### DEBUG # Appel progIrri() - Refresf contenuFlash() ###"));
  #endif
  contenuFlash();

  for (int nbrProg = 0; nbrProg <= nombreProgrammeDispo - 1; nbrProg++) {

    // *** Si ce n'est pas la première ligne, cloture le div précédent ***
    if (nbrProg != 0) {
      s += "</div>";
    }

    // *** Création de la ligne ***
    s += "<div class=\"row\" >";
    s += "<div class=\"col-sm-12\"><HR></div>";
    s += "</div>";

    s += "<div class=\"row ligneProg\" data-prog=" + String(nbrProg) + " data-v1=" + String(nbrProg) + " >";

    // *** Case <<Px>> numéro du programme ***
    s += "<div class=\"col-sm status" + String(progVanneStatus[nbrProg]) + "\" >P<B>" + String(nbrProg) + "</B><P> - ";
    for (int i=0; i<=(labelSize - 1); i++){
      s += char(progVanneLabel[nbrProg][i]);
    }
    s += " - </div>";

    // *** Case <<STATUS>> du programme ***
    s += "<div class=\"col-sm status" + String(progVanneStatus[nbrProg]) + "\"><B>";
    switch (progVanneStatus[nbrProg]) {
      case 0:
        s += "Non utilisé !";
        break;
      case 1:
        s += "Operationnel";
        break;
      case 2:
        s += "Operationnel ECO";
        break;
      default:
        s += "<SPAN style=\"background-color:#FF0000;\">Err. Flash</SPAN>";
        break;
    }
    s += "</B></div>";

    // *** Case hygrometrie de déclenchement ***
    #ifdef DebugProgIrri
      Serial.println(F("### DEBUG # progIrri() - progVanneHygrometrie[") + String(nbrProg) + "] : " + String(progVanneHygrometrie[nbrProg]) + " ###");
    #endif
  
    s += "<div class=\"col-sm status" + String(progVanneStatus[nbrProg]) + "\" ><B>";
    if (progVanneStatus[nbrProg] != 0) {
      s += String(progVanneHygrometrie[nbrProg]) + " %</B>";
      if (groundWet > progVanneHygrometrie[nbrProg]) {
        s += "<P>#### Sol trop humide ... <BR>Le programme ne se déclenchera pas ! ####"; 
      }
    }
    s += "</div>";

    // *** Case <<HH:MM>> ***
    s += "<div class=\"col-sm status" + String(progVanneStatus[nbrProg]) + "\">";
    if (progVanneStatus[nbrProg] != 0) {
      s += String(progVanneHDebut[nbrProg]) + "H" + progVanneHDebut[nbrProg] + "M";
    }
    s += "</div>";


    // *** case <<DUREE>> arrosage sur la base de l'horaire défini ***
    s += "<div class=\"col-sm status" + String(progVanneStatus[nbrProg]) + "\">";
    if (progVanneStatus[nbrProg] != 0) {
      s += String(progVanneDuree[nbrProg]) + " min<BR>";
      s += "Fin: " + String(progVanneHFin[nbrProg]) + ":" + String(progVanneMFin[nbrProg]);
    }
    s += "</div>";


    // *** Case <<JOUR ACTIFS>> ***
    s += "<div class=\"col-sm status" + String(progVanneStatus[nbrProg]) + "\">";

    if (progVanneStatus[nbrProg] != 0) {
      // *** Explore l'octet des jours pour affficher seulement les jours actifs
      int masque = 1;
      for (int j = 0; j <= 6; j++) {
        if ((masque & progVanneJours[nbrProg]) == masque) {
          //s += jourSemaineMins[j];
          s += jourSemaine[j] + "<BR>";
        }
        masque = masque << 1;
      }
    }
    s += "</div>";


    // *** Case <<VANNE>> ***
    s += "<div class=\"col-sm status" + String(progVanneStatus[nbrProg]) + " vanne\">";
    if (progVanneStatus[nbrProg] != 0) {
      s += "<B>";
      s += "<Span id=\"statusVanne" + String(nbrProg) + "\" name=\"statusV" + String(progVanneRef[nbrProg]) + "\">";
      s += "</Span></B>";
    }
    s += "</div>";
  }

  s += "</div>";
  
  s += "<div class=\"row\" >";
  s += "#(c)Numa 2023/07 - V" + String(versionFile);
  s += "</div>";
  


  //s += "</TABLE></form>";

  server.send(200, "text/html", s);
}



void handleNotFound() {
  Serial.println(F("##### 404 page #####"));
  server.send(404, "text/html", "404: Not found");  // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void readProgIrri() {
}
