#include "BluetoothSerial.h" 

// init Class:
BluetoothSerial ESP_BT; 

int Plantes [50][2];    //Matrice qui contient les paramètres des plantes
int Recette;            //Variable qui contient le numéro de la recette qu'on souhaite modifier ou ajouter à un emplacement
int PlaModif;           //Variable qui permet de choisir la plante à modifier
int HumModif;           //Variable qui contient la nouvelle valeur d'humidité pour la plante
int LumModif;           //Variable qui contient la nouvelle valeur de luminosité pour la plante
bool Modif;             //Variable qui confirme la modification et modifie la matrice
unsigned long rememberTimeHum=0;      //Variable qui garde en mémoire le temps pour les tempos d'humidité
extern volatile unsigned long timer0_millis;
unsigned long ZeroMillis=0;
int Emplacement1=0;     //Variable qui choisit la plante à l'emplacement 1
int Emplacement2=0;     //Variable qui choisit la plante à l'emplacement 2
int Emplacement3=0;     //Variable qui choisit la plante à l'emplacement 3
int Emplacement4=0;     //Variable qui choisit la plante à l'emplacement 4
int Emplacement5=0;     //Variable qui choisit la plante à l'emplacement 5
int Humidité1;          //Contient la valeur de l'humidité lue par le capteur 1
int Humidité2;          //Contient la valeur de l'humidité lue par le capteur 2
int Humidité3;          //Contient la valeur de l'humidité lue par le capteur 3
int Humidité4;          //Contient la valeur de l'humidité lue par le capteur 4
int Humidité5;          //Contient la valeur de l'humidité lue par le capteur 5
const int PatteCapteur1=1;        //Capteur d'humidité 1
const int PatteCapteur2=2;        //Capteur d'humidité 2
const int PatteCapteur3=3;        //Capteur d'humidité 3
const int PatteCapteur4=4;        //Capteur d'humidité 4
const int PatteCapteur5=5;        //Capteur d'humidité 5
bool Pompe1;            //État de la pompe 1
bool Pompe2;            //État de la pompe 2
bool Pompe3;            //État de la pompe 3
bool Pompe4;            //État de la pompe 4
bool Pompe5;            //État de la pompe 5
const int PattePompe1=10;     //Pompe 1
const int PattePompe2=10;     //Pompe 2
const int PattePompe3=10;     //Pompe 3
const int PattePompe4=10;     //Pompe 4
const int PattePompe5=10;     //Pompe 5
// Paramètre recu de l'interface Bluetooth
int BT_recu;
//Variables de contrôle pour les DELs
int DEL1_J;
int DEL1_R;
int DEL2_J;
int DEL2_R;
int DEL3_J;
int DEL3_R;
int DEL4_J;
int DEL4_R;
int DEL5_J;
int DEL5_R;

bool Minuit;            //Variable mis à 1 par l'API à minuit
bool État_Minuit;       //Permet la détection du front montant de Minuit

void setup() {
  //Initialisation de la communication Bluetooth
  Serial.begin(19200);
  ESP_BT.begin("ESP32_Plantes"); //Name of your Bluetooth interface -> will show up on your phone

}

void loop() {
  //Vérifie l'humidité des plantes, un étage à la fois avec une minute par étage
  if ((millis()- rememberTimeHum)<= 60000) {
    Plante1();
  }
    else if ((millis()- rememberTimeHum)<= 120000) {
      Plante2();
      }
      else if ((millis()- rememberTimeHum)<= 180000) {
        Plante3();
      }
        else if ((millis()- rememberTimeHum)<= 240000) {
          Plante4();
        }
          else if ((millis()- rememberTimeHum)<= 300000) {
            Plante5();
            rememberTimeHum=millis();
          }
if (ESP_BT.available()) 
  {
    BT_recu = ESP_BT.read(); //Read what we receive 
      
      switch (BT_recu) 	//Conversion des états des DEL sur 8 bits pour la communication vers l'API
      {
         case 11: Emplacement1=Recette;break;                    //Choix de la plante à l'emplacement 1
         case 12: Emplacement2=Recette;break;                    //Choix de la plante à l'emplacement 2
         case 13: Emplacement3=Recette;break;                    //Choix de la plante à l'emplacement 3
         case 14: Emplacement4=Recette;break;                    //Choix de la plante à l'emplacement 4
         case 15: Emplacement5=Recette;break;                    //Choix de la plante à l'emplacement 5
         case 101: Plantes[Recette][1]=1;break;                   //Humidité 1/10
         case 102: Plantes[Recette][1]=2;break;                   //Humidité 2/10
         case 103: Plantes[Recette][1]=3;break;                   //Humidité 3/10
         case 104: Plantes[Recette][1]=4;break;                   //Humidité 4/10
         case 105: Plantes[Recette][1]=2000;break;                //Humidité 5/10
         case 106: Plantes[Recette][1]=6;break;                   //Humidité 6/10
         case 107: Plantes[Recette][1]=7;break;                   //Humidité 7/10
         case 108: Plantes[Recette][1]=8;break;                   //Humidité 8/10
         case 109: Plantes[Recette][1]=9;break;                   //Humidité 9/10
         case 110: Plantes[Recette][1]=10;break;                  //Humidité 10/10
         case 200: Plantes[Recette][2]=0;break;                   //Luminosité 0/24
         case 201: Plantes[Recette][2]=1;break;                   //Luminosité 0/24
         case 202: Plantes[Recette][2]=2;break;                   //Luminosité 0/24
         case 203: Plantes[Recette][2]=3;break;                   //Luminosité 0/24
         case 204: Plantes[Recette][2]=4;break;                   //Luminosité 0/24
         case 205: Plantes[Recette][2]=5;break;                   //Luminosité 0/24
         case 206: Plantes[Recette][2]=6;break;                   //Luminosité 0/24
         case 207: Plantes[Recette][2]=7;break;                   //Luminosité 0/24
         case 208: Plantes[Recette][2]=8;break;                   //Luminosité 0/24
         case 209: Plantes[Recette][2]=9;break;                   //Luminosité 0/24
         case 210: Plantes[Recette][2]=10;break;                   //Luminosité 0/24
         case 211: Plantes[Recette][2]=11;break;                   //Luminosité 0/24
         case 212: Plantes[Recette][2]=12;break;                   //Luminosité 0/24
         case 213: Plantes[Recette][2]=13;break;                   //Luminosité 0/24
         case 214: Plantes[Recette][2]=14;break;                   //Luminosité 0/24
         case 215: Plantes[Recette][2]=15;break;                   //Luminosité 0/24
         case 216: Plantes[Recette][2]=16;break;                   //Luminosité 0/24
         case 217: Plantes[Recette][2]=17;break;                   //Luminosité 0/24
         case 218: Plantes[Recette][2]=18;break;                   //Luminosité 0/24
         case 219: Plantes[Recette][2]=19;break;                   //Luminosité 0/24
         case 220: Plantes[Recette][2]=20;break;                   //Luminosité 0/24
         case 221: Plantes[Recette][2]=21;break;                   //Luminosité 0/24
         case 222: Plantes[Recette][2]=22;break;                   //Luminosité 0/24
         case 223: Plantes[Recette][2]=23;break;                   //Luminosité 0/24
         case 224: Plantes[Recette][2]=24;break;                   //Luminosité 0/24
         case 301: Recette=1;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 302: Recette=2;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 303: Recette=3;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 304: Recette=4;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 305: Recette=5;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 306: Recette=6;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 307: Recette=7;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 308: Recette=8;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 309: Recette=9;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 310: Recette=10;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 311: Recette=11;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 312: Recette=12;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 313: Recette=13;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 314: Recette=14;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 315: Recette=15;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 316: Recette=16;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 317: Recette=17;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 318: Recette=18;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 319: Recette=19;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 320: Recette=20;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 321: Recette=21;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 322: Recette=22;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 323: Recette=23;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 324: Recette=24;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 325: Recette=25;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 326: Recette=26;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 327: Recette=27;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 328: Recette=28;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 329: Recette=29;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 330: Recette=30;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 331: Recette=31;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 332: Recette=32;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 333: Recette=33;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 334: Recette=34;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 335: Recette=35;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 336: Recette=36;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 337: Recette=37;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 338: Recette=38;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 339: Recette=39;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 340: Recette=40;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 341: Recette=41;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 342: Recette=42;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 343: Recette=43;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 344: Recette=44;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 345: Recette=45;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 346: Recette=46;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 347: Recette=47;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 348: Recette=48;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 349: Recette=49;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 350: Recette=50;break;                                //Sélection de la recette 1 pour modification ou emplacement
         case 411: DEL1_J=49;DEL1_R=48;break;                       //Controle du mode lumineux pour l'emplacement 1 (En ASCII pour l'API)
         case 412: DEL1_J=48;DEL1_R=49;break;
         case 413: DEL1_J=49;DEL1_R=49;break;
         case 421: DEL2_J=49;DEL2_R=48;break;                       //Controle du mode lumineux pour l'emplacement 2 (En ASCII pour l'API)
         case 422: DEL2_J=48;DEL2_R=49;break;
         case 423: DEL2_J=49;DEL2_R=49;break;
         case 431: DEL3_J=49;DEL3_R=48;break;                       //Controle du mode lumineux pour l'emplacement 3 (En ASCII pour l'API)
         case 432: DEL3_J=48;DEL3_R=49;break;
         case 433: DEL3_J=49;DEL3_R=49;break;
         case 441: DEL4_J=49;DEL4_R=48;break;                       //Controle du mode lumineux pour l'emplacement 4 (En ASCII pour l'API)
         case 442: DEL4_J=48;DEL4_R=49;break;
         case 443: DEL4_J=49;DEL4_R=49;break;
         case 451: DEL5_J=49;DEL5_R=48;break;                       //Controle du mode lumineux pour l'emplacement 5 (En ASCII pour l'API)
         case 452: DEL5_J=48;DEL5_R=49;break;
         case 453: DEL5_J=49;DEL5_R=49;break;
      }
  }

  if ((Minuit != État_Minuit)AND(Minuit==1){
    setMillis(ZeroMilis);
  }
DigitalWrite(PattePompe1, Pompe1);
DigitalWrite(PattePompe2, Pompe2);
DigitalWrite(PattePompe3, Pompe3);
DigitalWrite(PattePompe4, Pompe4);
DigitalWrite(PattePompe5, Pompe5);
État_Minuit=Minuit;
}

void Plante1() {
  // Code qui fait la gestion de l'humidité de la plante à l'étage 1
  //Remise à zéro des autres pompes
  Pompe2=LOW;Pompe3=LOW;Pompe4=LOW;Pompe5=LOW;
  // Lecture du capteur
  Humidité1 = analogRead(Capteur1);
  // Contrôle de la pompe en fonction de l'humidité
  if (Humidité1 < Plantes[Emplacement1][1]) {
    // Si le taux d'humidité est bas, activer la pompe
    Pompe1=HIGH;
  }
  if (Humidité1 > (Plantes[Emplacement1][1])+200) {
    // Si le taux d'humidité est bas, activer la pompe
    Pompe1=LOW;
  }
// Annuler l'activation de la pompe si aucun courant ne traverse 
// Le circuit est ouvert. Le capteur est sûrement détérré
  if (Humidité1==0) {
    Pompe1=LOW;
}
void Plante2() {
  // Code qui fait la gestion de l'humidité de la plante à l'étage 2
    //Remise à zéro des autres pompes
  Pompe1=LOW;Pompe3=LOW;Pompe4=LOW;Pompe5=LOW;
  // Lecture du capteur
  Humidité2 = analogRead(Capteur2);
  // Contrôle de la pompe en fonction de l'humidité
  if (Humidité2 < Plantes[Emplacement2][1]) {
    // Si le taux d'humidité est bas, activer la pompe
    Pompe2=HIGH;
  }
  if (Humidité2 > (Plantes[Emplacement2][1])+200) {
    // Si le taux d'humidité est bas, activer la pompe
    Pompe2=LOW;
  }
// Annuler l'activation de la pompe si aucun courant ne traverse 
// Le circuit est ouvert. Le capteur est sûrement détérré
  if (Humidité2==0) {
    Pompe2=LOW;
}
void Plante3() {
  // Code qui fait la gestion de l'humidité de la plante à l'étage 3
    //Remise à zéro des autres pompes
  Pompe2=LOW;Pompe1=LOW;Pompe4=LOW;Pompe5=LOW;
  // Lecture du capteur
  Humidité3 = analogRead(Capteur3);
  // Contrôle de la pompe en fonction de l'humidité
  if (Humidité3 < Plantes[Emplacement3][1]) {
    // Si le taux d'humidité est bas, activer la pompe
    Pompe3=HIGH;
  }
  if (Humidité3 > (Plantes[Emplacement3][1])+200) {
    // Si le taux d'humidité est bas, activer la pompe
    Pompe3=LOW;
  }
// Annuler l'activation de la pompe si aucun courant ne traverse 
// Le circuit est ouvert. Le capteur est sûrement détérré
  if (Humidité3==0) {
    Pompe3=LOW;
}
void Plante4() {
  // Code qui fait la gestion de l'humidité de la plante à l'étage 4
}
void Plante5() {
  // Code qui fait la gestion de l'humidité de la plante à l'étage 5
}
void setMillis(unsigned long  new_millis) {
  // Code qui remet millis() à zéro
  uint8_t oldSREG = SREG;
  cli();
  timer0_millis = new_millis;
  SREG = old SREG;
}
