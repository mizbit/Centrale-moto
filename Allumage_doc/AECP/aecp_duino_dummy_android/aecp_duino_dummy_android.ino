#include <TimerOne.h> //Génère une interruption toutes les  trech ou Dsecu µs
#include <SoftwareSerial.h>
//**************************************************************
//Aecp-Duino    Allumage electronique programmable - Arduino - 2016
//Mr Loutrel Philippe, voir son site,
//http://a110a.free.fr/SPIP172/article.php3?id_article=142
//Mr Dedessus Les Moutier Christophe pour la version Panhard
char ver[] = "version du 18_04_2017";//Choix entre 3 types de Dwell.
//En option, connexion d'un sélecteur entre la patte A4 et A5 et la masse
//pour changer de courbe d'avance centrifuge et dépression
//Les datas envoyées pour Processing sont en fin de boucle loop 
//T>14ms, correction Christophe.Avance 0°jusqu'a 500t/mn, anti retour de kick
//Detection et traitement special au demarrage (première etincelle)
//******************************************************************************
//Le module BlueTooth est le très courant HC 06 à moins de 5€
//Connecter la patte TX du module à D11, la patte RX du module à D12
//Connecter les masses et le Vcc du module au +5V du 7805 
//Mettre le module BlueTooth en mode AT( pin 34 au +5V ) et entrer AT+UART=38400,1,0
//38400 bps entre module et smartphone
//Installer l'appli BlueTerm , connecter: sur l'ecran l'avance enn degrés est affichée

SoftwareSerial BT(11, 12); // RX| TX vers le module BlueTooth HC05/06

//************* ces lignes explique la lecture de la dépression ****************
// Pour la dépression ci-dessous le tableau des mesures relevés sur un banc Souriau
// Degdep = map((analogRead(A0)),xhigh,xlow,yhigh,ylow);
int xhigh = 0;// valeur ADC haute pour conversion de la valeur mmHg haute
int xlow = 0;// valeur ADC basse pour conversion de la valeur mmHg basse
int yhigh = 0;// valeur de la limite en degré haute x 10 pour avoir les centièmes
int ylow = 0;// valeur de la limite en degré basse soit 0 degré
// tableau des valeurs mmHg vs 8 bits
// {0,707}{25,667}{50,620}{60,601}{75,576}{80,565}{95,538}{100,530}{125,485}{150,438}{175,394}{185,373}{200,350}{210,330}{225,305}{250,260}{275,216}{300,172}
// relevé en activant la ligne Degdepcal et en ajoutant cette variable à l'édition dans le terminal série
// on peut en déduire une équation qui donne la valeur en mmHg au départ de la conversion ADC , utilisé sous Processing
// mmHg = (707 - valeur ADC)/1.78 ou 1.78 est la pente du capteur pour tension VS mmHg

//**************  Ces 6 lignes sont à renseigner obligatoirement.****************
// Ce sont : Na[],Anga[],  Ncyl, AngleCapteur, CaptOn, Dwell
//Les tableau Na[] et Anga[] doivent obligatoirement débuter puis se terminer par  0
//et  contenir des valeurs  entières >=1
//Le nombre de points est libre.L'avance est imposée à 0° entre 0 et Nplancher t/mn
//Le dernier N fixe la ligne rouge, c'est à dire la coupure de l'allumage
int Na[] = {0,1240,2050,2500,3400,4100,4200,7000, 0};
//degrés d'avance vilebrequin correspondant soit en tour volant moteur ( attention ! ):
int Anga[] = {0,0,4,6,10,14,16,16, 0};

int arrayVitesse[] = {1440,1320,1360,1240,1240,1240,1200,1200,1240,1160,1200,1200,1160,1200,1200,1120,1200,1640,2120,2000,1960,1880,1720,1560,1600,1920,2240,2480,
2800,2960,3280,3600,3760,4000,4240,4360,3840,3320,2840,2520,2200,2000,2080,2200,2200,2240,2280,2240,2160,2160,2040,1920,1800,1720,1720,1680,1480,1640,1600,1680,1720,
1800,1920,1960,2040,2120,2160,2320,2360,2440,2440,2360,2400,2320,2320,2240,2240,2160,2120,2080,1960,1880,1760,1600,1480,1360,1360,1320,1320,1200,1160,1240,1320,1760,
1880,1920,1800,1640,1560,1880,2120,2280,2640,3260,3820,4240,4960,5580,6160,6260,5760,5280,4840,3480,2840,2040,2160,2200,2280,2320,2400,2520,2600,2640,2760,2800,2800,
2840,2840,2920,2920,3000,3080,3080,3040,3080,3040,3080,3000,2960,2920,2840,2840,2720,2600,2480,2360,2280,2200,2120,2080,2000,1880,1800,1600};
float arrayDep[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,15,13,6,6,7,5,7,7,7,7,5,5,5,5,6,4,4,6,0,0,0,0,15,7,5,7,9,14,15,0,0,0,0,0,0,0,0,0,0,13,4,4,3,0,0,0,0,1,1,3,7,
8,15,3,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,5,4,3,2,3,5,3,1,2,3,4,3,3,0,0,0,0,0,15,6,5,1,1,3,4,2,1,3,3,7,15,15,6,5,6,9,15,14,15,15,15,15,15,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,};

int Ncyl = 2;           //Nombre de cylindres, moteur Panhard
const int AngleCapteur = 90; //le capteur(Hall) est 45° avant le PMH habituellement
const int Avancestatique = 17; // Correspond à 5 dents d'avance statique sur volant moteur
const int CaptOn = 0;  //CapteurOn = 1 déclenche sur front montant (capteur saturé)
//CapteurOn = 0 déclenche sur front descendant (capteur non saturé).Voir fin du listing
//**********************************************************************************
const int Dwell = 1; //1 pour alimenter la bobine en permanence sauf 1ms/cycle.Elle doit pouvoir chauffer sans crainte
// 2 pour alimentation de la bobine seulement trech ms par cycle, 3ms par exemple,
//indispensable pour bobine 'electronique' ou  de faible resistance: entre 2 et 0.5ohm
//3 pour simuler un allumage à vis platinées: bobine alimentée 2/3 (66%) du cycle.
//************************************************************************************
//*******************MULTICOURBES****IL FAUT TOURNER LE SELECTEUR!!!!!!!*******
//A la place de la courbe a, on peut selectionner la courbe b, c, d ou la e
//Un sélecteur rotatif et 4 résistances de 4K7, 18K, 47K et 100K font le job
//avec l'entrée configurée en Input Pull-up
//*******//*********Courbe   b
int Nb[] = {0,1600,4300,7000, 0};   //Courbe b
int Angb[] = {0,0,14,14, 0};
//*******//*********Courbe   c
int Nc[] = {0,600,4000,7000, 0};    //Courbe c
int Angc[] = {0,0,18,18, 0};
//*******//*********Courbe   d
int Nd[] = {0,1600,4000,7000, 0};   //Courbe d
int Angd[] = {0,0,18,18, 0};
//*******//*********Courbe   e
int Ne[] = {0,600,4300,7000, 0};    //Courbe e
int Ange[] = {0,0,14,14, 0};
//**********************************************************************************
//************Ces 4 valeurs sont eventuellement modifiables*****************
//Ce sont Nplancher, trech , Dsecu et delAv
const int Nplancher = 500; // vitesse en t/mn jusqu'a laquelle l'avance  = 0°
const int trech  = 3000;//temps de recharge bobine, 3ms= 3000µs typique
const int unsigned long Dsecu  = 1000000;//Securite: bobine coupee à l'arret apres Dsecu µs
int delAv = 1;//delta avance,par ex 2°. Quand Pot avance d'une position, l'avance croit de delAv

const int Bob1 = 8;    //Sortie D8 vers bobine1
const int Bob2 = 4;    //Sortie D4 vers bobine2
const int Cible = 2;  //Entrée sur D2 du capteur, R PullUp et interrupt
const int Pot = A4;   //Entrée analogique sur A4 pour potard de changement de courbes. R PullUp
const int Led = 13; //Sortie D13 avec la led built-in pour caller le disque par rapport au capteur 
int unsigned long D = 0;  //Delai en µs à attendre après la cible pour l'étincelle
int unsigned long Ddep = 0;
int unsigned long Dsum = 0;
int valPot = 0;       //0 à 1023 selon la position du potentiomètre en entree
int milli_delay = 0;
int micro_delay = 0;
float RDzero = 0; //pour calcul delai avance 0° < vitesse seuil plancher
float  Tplancher = 0; //idem
const int tcor  = 380; //correction en µs  du temps de calcul pour D 120µs + 120µs de lecture de dépression + 140µs de traitement
int unsigned long Davant_rech = 0;  //Delai en µs après etincelle pour demarrer la recharge bobine.
int unsigned long Tprec  = 0;//Periode precedant la T en cours, pour calcul de Drech
int unsigned long prec_H  = 0;  //Heure du front precedent en µs
int unsigned long T  = 0;  //Periode en cours
int N1  = 0;  //Couple N,A de debut d'un segment
int Ang1  = 0; // Car A1 reservé pour entrée analogique!
int N2  = 0; //Couple N,A de fin de segment
int Ang2  = 0;
int*  pN = &Na[0];//pointeur au tableau des régimes. Na sera la courbe par defaut
int*  pA = &Anga[0];//pointeur au tableau des avances. Anga sera la  courbe par defaut
float k = 0;//Constante pour calcul de l'avance courante
float C1[20]; //Tableaux des constantes de calcul de l'avance courante
float C2[20]; //Tableaux des constantes de calcul de l'avance courante
float Tc[20]; //Tableau des Ti correspondants au Ni
//Si necessaire, augmenter ces 3 valeurs:Ex C1[30],C2[30],Tc[30]
int Tlim  = 0;  //Période minimale, limite, pour la ligne rouge
int j_lim = 0;  //index maxi des N , donc aussi  Ang
int unsigned long NT  = 0;//Facteur de conversion entre N et T à Ncyl donné
int AngleCibles = 0;//Angle entre 2 cibles, 180° pour 4 cyl, 120° pour 6 cyl, par exemple
int UneEtin = 1; //=1 pour chaque étincelle, testé par isr_CoupeI et remis à zero
int Ndem = 60;//Vitesse estimée du vilo entrainé par le demarreur en t/mn
int unsigned long Tdem  = 0;  //Periode correspondante à Ndem,forcée pour le premier tour
int Mot_OFF = 0;//Sera 1 si moteur detecté arrété par l'isr_GestionIbob()
int Vitesse = 0; //Vitesse de rotation moteur
float   AV = 0; //Avance en degrès pour transmission vers module BlueTooth
float   AVtot = 0; //Avance totale en degrés pour transmission vers module BlueTooth
String Message = ""; //Données renvoyer au smartphone ou pc via bluetooth ou usb

float uspardegre = 0;
int Dep = 0;
float Degdep = 0;

float Delaideg  = 0;  //µs/deg pour la dépression
// Tableau pour sauver des données temporelle
unsigned long Stop_temps;
unsigned long Tempsecoule = 0;

// Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

//Macro  ps(v) de debug pour imprimer le numero de ligne, le nom d'une variable, sa valeur
//puis s'arreter definitivement .
#define ps(v) Serial.print("Ligne_") ; Serial.print(__LINE__) ; Serial.print(#v) ; Serial.print(" = ") ;Serial.println((v)) ; Serial.println("  Sketch stop"); while (1);
//Exemple, à la ligne 140, l'instruction     ps(var1);
//inprimera  "Ligne_140var1 = 18  Sketch stop"

//Macro  pc(v)de debug pour imprimer le numero de ligne, le nom d'une variable, sa valeur,
//puis s'arreter et attendre un clic de souris  sur le bouton 'Envoyer'en haut de l'ecran seriel pour continuer.
#define pc(v) Serial.print("Ligne_") ; Serial.print(__LINE__) ; Serial.print(#v) ;Serial.print(" = ") ; Serial.println((v)) ; Serial.println(" Clic bouton 'Envoyer' pour continuer") ;while (Serial.available()==0);{ int k_ = Serial.parseInt() ;}
//Exemple, à la ligne 145, l'instruction    pc(var2);
// inprimera   "Ligne_145var2 = 25.3   Clic bouton 'Envoyer' pour continuer"

int deb = 0; //0 sinon 1 ou 2 pour le debugging,et executer les impressions pc() ou pc(). Voir Setup()

//********************LES FONCTIONS*************************

void  CalcD ()////////////////////while (1); delay(1000);/////////////////////////////
// Noter que T1>T2>T3...
{ for (byte j = 1; j <= j_lim; j++)//On commence par T la plus longue et on remonte
  { //Serial.print("Tc = "); Serial.println(Tc[j]);delay(1000);
    if  (T >=  Tc[j]) {     //on a trouvé le bon segment de la courbe d'avance
      D = float(T * C1[j]  + C2[j]);//D en µs, C2 est déjà diminué du temps de calcul~50mus
      if ( T > Tplancher)D = T * RDzero;
      //break;  //Sortir, on a D
      Ddep = (Degdep * Delaideg ) ;//Ddep en µs
      Dsum = D - Ddep - tcor ;// tcor appliqué ici donne de meilleurs résultats niveau précision
      //if (deb > 1) {  pc(C1[j] );  pc(C2[j] );pc(D);}
      break;
    }
  }
}
void  Etincelle ()//////////while (1); delay(1000);/////////////////////////////
//{gf = 1; while (gf < D/14)gf++;//10= 100µs,100=1.1ms,2000=21.8ms//attente possible
{ 
  if (Dsum < 6500){
    delayMicroseconds(Dsum);} 
  else {
  milli_delay = ((Dsum/1000)-2);  //Quand D >10ms car problèmes avec delayMicroseconds(D) si D>14ms!
  micro_delay = (Dsum-(milli_delay*1000)); // 
  delay(milli_delay); // Currently, the largest value that will produce an accurate delay is 16383 µs
  delayMicroseconds(micro_delay); 
  }
  digitalWrite(Bob1, 0); //Couper le courant, donc étincelle
  digitalWrite(Bob2, 0); //Couper le courant, donc étincelle
  Stop_temps = micros();
  switch (Dwell)  //Attente courant coupé selon le type de Dwell

  { case 1:       //Ibob coupe 1ms par cycle seulement, la bobine peut chauffer
      Davant_rech = 1000; //1ms off par cycle
      break;

    case  2:      //Type bobine faible resistance, dite "electronique"
      Davant_rech = 2 * T - Tprec - trech; //Tenir compte des variations de régime moteur
      Tprec = T;    //Maj de la future periode precedente
      break;

    case  3:      //Type "vis platinées", Off 1/3, On 2/3
      Davant_rech = T / 3;
      break;
  }
  Timer1.initialize(Davant_rech);//Attendre Drech µs avant de retablire le courant dans la bobine
  UneEtin = 1; //Signaler une étincelle à l'isr_GestionIbob().
  
}
void  Init ()////////////////////while (1); delay(1000);/////////////////////////////
{ AngleCibles = 720 / Ncyl; //Ex pour 4 cylindres 180°, et 120° pour 6 cylindres
  NT  = 120000000 / Ncyl; //Facteur de conversion Nt/mn en Tµs
  Tdem  = NT/Ndem;//Periode de la première étincelle
  Tplancher = 120000000 / Nplancher / Ncyl; //T à  vitesse plancher en t/mn, en dessous, avance centrifuge = 0
  RDzero = float(AngleCapteur - Avancestatique) / float(AngleCibles);
  Select_Courbe();  //Ajuster éventuellement les pointeurs pN et pA pour la courbe b ou c 
  N1  = 0; Ang1 = 0; //Toute courbe part de  0
  int i = 0;    //locale mais valable hors du FOR
  pN++; pA++; //sauter le premier element de tableau, toujours =0
  for (i  = 1; *pN != 0; i++)//i pour les C1,C2 et Tc.Arret quand regime=0.
  //pN est une adresse (pointeur) qui pointe au tableau N.Le contenu pointé est *pN
  { N2 = *pN; Ang2 = *pA;//recopier les valeurs pointées dans N2 et Ang2
    k = float(Ang2 - Ang1) / float(N2  - N1);
    C1[i] = float(AngleCapteur - Avancestatique - Ang1 + k * N1) / float(AngleCibles);
    C2[i] = -  float(NT * k) / float(AngleCibles);
    Tc[i] = float(NT / N2);
    N1 = N2; Ang1 = Ang2; //fin de ce segment, début du suivant
    pN++; pA++;   //Pointer à l'element suivant de chaque tableau
  }
  j_lim = i - 1; //Revenir au dernier couple entré
  Tlim  = Tc[j_lim]; //Ligne rouge
  Serial.print("Tc = "); for (i = 1 ; i < 12; i++)Serial.println(Tc[i]);
  Serial.print("Tlim = "); Serial.println(Tlim);
  Serial.print("C1 = "); for (i = 1 ; i < 12; i++)Serial.println(C1[i]);
  Serial.print("C2 = "); for (i = 1 ; i < 12; i++)Serial.println(C2[i]);

  //Timer1 a deux roles:
  //1....couper le courant dans la bobine en l'absence d'etincelle pendant plus de Dsecu µs
  //2... après une étincelle, attendre le delais Drech avant de retablir le courant dans la bobine
  //Ce courant n'est retabli que trech ms avant la prochaine étincelle, condition indispensable
  //pour une bobine à faible resistance, disons inférieure à 3 ohms.Typiquement trech = 3ms.
  Timer1.attachInterrupt(isr_GestionIbob);//IT d'overflow de Timer1 (16 bits)
  Timer1.initialize(Dsecu);//Le courant dans la bobine est coupé si aucune etincelle durant Dsecu µs
  Mot_OFF = 1;
  digitalWrite(Bob1, 0);//par principe, couper la bobine
  digitalWrite(Bob2, 0);//par principe, couper la bobine
}
void  isr_GestionIbob()////////////////////while (1); delay(1000);/////////////////////////////
{ Timer1.stop();    //Arreter le decompte du timer
  if (UneEtin == 1){
  digitalWrite(Bob1, 1);  //Retablire le courant dans bobine
  digitalWrite(Bob2, 1);  //Retablire le courant dans bobine
  }
  else
  { digitalWrite(Bob1, 0);//Preserver la bobine, couper le courant
    digitalWrite(Bob2, 0);//Preserver la bobine, couper le courant
    Mot_OFF = 1;//Permettra à loop() de detecter le premier front de capteur
  }
  UneEtin = 0;  //Remet  le detecteur d'étincelle à 0
  Timer1.initialize(Dsecu);//Au cas où le moteur s'arrete, couper la bobine apres Dsecu µs
}
void  Select_Courbe()////////////while (1); delay(1000);/////////////////////////////
//Par défaut, la courbe a est déja selectionnée
{ valPot = analogRead(Pot);
  Serial.print("valPot = "); Serial.print(valPot);
  if (valPot < 99) {                  // Shunt 0 ohm donne 15 
    Serial.println(", Courbe a");
  }
  if (valPot > 110 && valPot < 150) { // Résistance de 4K7 donne 130 
    pN = &Nb[0];  // pointer à la courbe b
    pA = &Angb[0];
    Serial.println(", Courbe b");
  }
  if (valPot > 320 && valPot < 360) {  // Résistance de 18K donne 340    
    pN = &Nc[0];  // pointer à la courbe c
    pA = &Angc[0];
    Serial.println(", Courbe c");
  }
  if (valPot > 545 && valPot < 585) {  // Résistance de 47K donne 565    
    pN = &Nd[0];  // pointer à la courbe c
    pA = &Angd[0];
    Serial.println(", Courbe d");
  }
  if (valPot > 715 && valPot < 755) {  // Résistance de 100K donne 735    
    pN = &Ne[0];  // pointer à la courbe c
    pA = &Ange[0];
    Serial.println(", Courbe e");
  }
  if (valPot > 995) {                  // Pas de shunt donne 1015 
    Serial.println(", Courbe a");
  }
}
////////////////////////////////////////////////////////////////////////
void setup()//////////////////while (1); delay(1000);//////////////////////////
/////////////////////////////////////////////////////////////////////////
{ deb = 0; //pour debugging, 1 ou 2 sinon 0
  Serial.begin(115200);//Ligne suivante, 3 Macros du langage C
  BT.begin(115200);//Vers module BlueTooth HC05/06
  BT.flush();//A tout hasard Ligne suivante, 3 Macros du langage C
  Serial.println(__FILE__); Serial.println(__DATE__); Serial.println(__TIME__);
  Serial.println(ver);
  if (Ncyl < 2)Ncyl = 2; //On assimile le mono cylindre au bi, avec une étincelle perdue
  pinMode(Cible, INPUT_PULLUP); //Entrée interruption sur D2, front descendant
  pinMode(Bob1, OUTPUT); //Sortie sur D4 controle du courant dans la bobine1
  pinMode(Bob2, OUTPUT); //Sortie sur D6 controle du courant dans la bobine2
  //Nota: on peut connecter une Led de controle sur D4 avec R=330 ohms vers la masse
  pinMode(Pot, INPUT_PULLUP); //Entrée pour potard 100kohms ou sélecteur rotatif avec résistances, optionnel !
  // set up the ADC
  ADCSRA &= ~PS_128;  // remove bits set by Arduino library
  // you can choose a prescaler from above.
  // PS_16, PS_32, PS_64 or PS_128
  ADCSRA |= PS_64;    
  // set our own prescaler to 64 
  Init(); // Executée une fois au demarrage et à chaque changement de courbe
  pinMode(Led, OUTPUT); // pour signaler le calage du capteur lors de la mise au point
}


void demoBTsmartphone(){
  for(int i = 1; i < 154; i++){
    Vitesse = arrayVitesse[i];
    T = NT/(Vitesse) ;
    uspardegre = (T/float(AngleCibles));
    CalcD();
    //Degdep = arrayDep[i];
    Dep = analogRead(A0);
    Degdep = map(Dep,330,565,300,0);  //Mesure la dépression
    Degdep = Degdep/10;
    //int Degdepcal = analogRead(A0); Pour calibrage du capteur MV3P5050
    if (Degdep < 0){ Degdep = 0; }
    else if (Degdep > 30){ Degdep = 30; }
    else ;
    AV = AngleCapteur - (D /float(uspardegre)) - Avancestatique ;
    AVtot = AV + Degdep + Avancestatique;
    Message = ("S,"+String(Vitesse)+(",")+String(Degdep,1)+(",")+String(AV, 1)+(",")+String(AVtot,1)+(",")+String(Dep)+(","));
    
     BT.println(Message);
     
    delay(200);
  }
  for(int i = 154; i > 0; i--){
    Vitesse = arrayVitesse[i];
    T = NT/(Vitesse) ;
    uspardegre = (T/float(AngleCibles));
    CalcD();
    //Degdep = arrayDep[i];
    Dep = analogRead(A0);
    Degdep = map(Dep,330,565,300,0);  //Mesure la dépression
    Degdep = Degdep/10;
    //int Degdepcal = analogRead(A0); Pour calibrage du capteur MV3P5050
    if (Degdep < 0){ Degdep = 0; }
    else if (Degdep > 30){ Degdep = 30; }
    else ;
    AV = AngleCapteur - (D /float(uspardegre)) - Avancestatique ;
    AVtot = AV + Degdep + Avancestatique;
     
     Message = ("S,"+String(Vitesse)+(",")+String(Degdep,1)+(",")+String(AV, 1)+(",")+String(AVtot,1)+(",")+String(Dep)+(","));
    
     BT.println(Message);
     
    delay(200);
  }
}
///////////////////////////////////////////////////////////////////////////
void loop()   /////////////////////while (1); delay(1000);/////////////////
////////////////////////////////////////////////////////////////////////////
{ 

   demoBTsmartphone();
}
////////////////DEBUGGING////////////////////////
//Voir les macros ps ()à et pc() en début de listing

//Une autre possibilité est de générer ou non du code de debug à la compilation.
//#define DEBUG    // #if defined DEBUG #endif

//Hertz = Nt/mn / 30 , pour moteur 4 cylindre
//N 1000,1500,2000,2500,3000,3500,4000,4500,5000,5500,6000,6500,7000,7500,8000,8500,9000,9500,10000
//Hz  33, 50,   66,  83  100  117   133 150   166 183 200    216  233 250   266 283   300 316 333

//Hertz = Nt/mn / 60 , pour moteur 2 cylindre
//N 1000,1500,2000,2500,3000,3500,4000,4500,5000,5500,6000,6500,7000,7500,8000,8500,9000,9500,10000
//Hz  16,  25,  33,  41   50   58   66   75   83   91  100  108  116  125  133  141  150  158   166
//Capteur Honeywell 1GT101DC,contient un aimant sur le coté,type non saturé, sortie haute à vide,
//et basse avec une cible en métal. Il faut  CapteurOn = 0, declenche sur front descendant.
//Le capteur à fourche SR 17-J6 contient un aimant en face,type saturé, sortie basse à vide,
//et haute avec une cible métallique. Il faut  CapteurOn = 1, declenche sur front montant.

