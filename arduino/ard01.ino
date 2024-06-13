/*
  FAETERJ/CPTI PETRÓPOLIS - SirLab - COMANDO DE FECHADURA ELETRONICA POR TECLADO 4x4
  Versão 3.0 - USANDO MICRO SD NO ETHERNET SHIELD PARA ARMAZENAR SENHAS - NÚMERO ILIMITADO DE USUÁRIOS
  NESTA VERSÃO FOI RETIRADA A INDEXAÇÃO DOS STRINGS DE SENHA E USUÁRIO LIDOS DO ARQUIVO PARA LIBERAR MEMÓRIA DO ARDUINO
  GRAVAÇÃO DO STRING DATA/HORA E NOME DO USUÁRIO NO ARQUIVO log.txt
  =================================================================
  Atualizado e modificado por Alan C Giese - 11/08/2021
  OBS: Sem o MicroSD com o arquivo de senhas o sketch não funciona pois precisa do mesmo para ler as senhas/usuários.
  ------------------------------------------------------------------------------------------------------------------
   FORMATO DO ARQUIVO bdSirLab.txt no Micro SD
   1234JoãoDaSilva   <--- UM USUÁRIO POR LINHA, ONDE OS 4 PRIMEIROS CARACTERES SÃO A SENHA
   3#B9MarioRocha
   .
   .
   .

   APÓS O ÚLTIMO USUÁRIO DAR UM ENTER E DEIXAR A ÚLTIMA LINHA EM BRANCO PARA SINALIZAR O FIM DO ARQUIVO
   -------------------------------------===============================--------------------------------
*/

#include <Keypad.h>
#include <SPI.h>
#include <SD.h>

#include <DS3231.h>
// >>>>>>>>>>>>>>>>>> ATENÇÃO: COMANDO ABAIXO SÓ SERVE PARA A BIBLIOTECA DS3231.h <<<<<<<<<<<<<<<<<<<<
DS3231 rtc(SDA, SCL);     // NO ARDUINO UNO CONECTAR: PINO 16 => SDA do RTC; PINO 17 => SCL do RTC
//                           TAMBÉM FUNCIONA LIGANDO NOS PINOS SDA E SCL NOS PINOS A4 e A5 do Arduino
// ---------------------------------------------------------------------------------------------------

File myFile;

#define modoDebug 0                   // Se modoDebug = 1 executa todos os Serial.print
//                                    // Se modoDebug = 2 avisa que gravou no arquivo log.txt
#define pinRele   A0                  // pino do Rele
#define pinBuzzer A1                  // pino do buzzer ativo
#define pinLedVm  A2                  // pino do led vermelho
#define pinLedVd  A3                  // pino do led Verde
#define passDigits 4                       // digitos da password
const byte numRows  = 4;                   // numero de linhas do keypad
const byte numCols  = 4;                   // numero de colunas do keypad

// OBS:   TECLAS: NUMÉRICAS: SÃO AS MESMAS;  -  DEMAIS TECLAS: A=17; B=18; C=19; D=20; *=-6; #=-13

int a = 0, b = 0, c = 0, d = 0;         // Inteiros que vão armazenar a password
byte numReg = 0;                        // numero de registros encontrados no arquivo de usuarios apos a leitura do arquivo
byte var = 0;                           // Variavel auxiliar; qdo for = 4 terminou a digitação da senha

// a matriz abaixo define a tecla pressionada de acordo com a linha e coluna como aparece no keypad
char keymap[numRows][numCols] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinLin1 = 10, pinLin2 = 9, pinLin3 = 8, pinLin4 = 7;             // portas das linhas
byte pinCol1 = 6, pinCol2 = 5, pinCol3 = 3, pinCol4 = 2;              // portas das colunas

byte rowPins[numRows] = {pinLin1, pinLin2, pinLin3, pinLin4};        //vetor de linhas (Rows)
byte colPins[numCols] = {pinCol1, pinCol2, pinCol3, pinCol4};        //vetor de colunas (Columns)

//     -------inicializa a instancia da classe Keypad -------
Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);

void setup() {
  Serial.begin(9600);
#if modoDebug == 2
  Serial.println("Inicio");
#endif

  
  rtc.begin();
  pinMode(pinLedVd, OUTPUT);
  pinMode(pinLedVm, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinRele, OUTPUT);
  digitalWrite(pinRele, HIGH);              // este rele é desativado com HIGH e ativado com LOW
#if modoDebug == 2
  Serial.println("Pinos Configurados");
#endif  

  // ******* PARA ACERTAR TIME (SÓ PARA A BIBLIOTECA DS3231.h DESCOMENTE AS 3 LINHAS ABAIXO *******
  //rtc.setDOW(SATURDAY);     // Set Day-of-Week to (Ex. FRIDAY)
  //rtc.setTime(9, 18, 0);     // Set the time to H:M:S (24hr format)
  //rtc.setDate(18, 9, 2021);   // Set the date to D,M,Y
  // **********************************************************************************************

  mostraData();                             // mostra data e hora no monitor serial

  // ----------------------------------------------------------
  while (!Serial) {
    ;                     // AGUARDA A PORTA SERIAL SE CONECTAR
  }
  // ----------------------------------------------------------

  inicializaSD();
  testaLeds();
}                                       // FIM DO SETUP

void loop() {

  digitalWrite(pinRele, HIGH);               // este rele é desativado com HIGH e ativado com LOW
  digitalWrite(pinLedVm, HIGH);              // led vermelho sempre aceso para mostrar que o sistema está ativo
  char keypressed = myKeypad.getKey();       // Se uma tecla é pressionada é armazenada na variável 'keypressed'
  String usuS;                               // nomes dos usuarios (lê do arquivo mas não armazena para economizar espaço)
  String passLida;                           // passwords com 4 posicoes alfanumericas (lê do arquivo mas não armazena para economizar espaço)
  String passDigitada;                       // password digitada
  String dataHora;

  dataHora = rtc.getDateStr();
  dataHora += "-";
  dataHora += rtc.getTimeStr();

#if modoDebug == 2
  Serial.println(dataHora);
#endif

  if (keypressed != NO_KEY)  {

    piscaLedVm(1, 100);                     // pisca o led vermelho e aciona buzzer (numero de vezes, tempo entre cada piscada)

    keypressed = keypressed - 48;

    int k2 = (int)keypressed;

    if (k2 != -6) {                                  // se digitar qq tecla valida, se digitar * anula no else
      var++;                                         // incremento auxiliar para armazenar os caracteres digitados
      switch (var) {
        case 1:
          a = k2;                                    // armazena o primeiro digito teclado no keypad
          break;
        case 2:
          b = k2;                                    // armazena o segundo digito teclado no keypad
          break;
        case 3:
          c = k2;                                    // armazena o terceiro digito teclado no keypad
          break;
        case 4:
          d = k2;                                    // armazena o quarto digito da teclado no keypad
          delay(100);

          // *******************************************************************************************

          if (var == passDigits) {                   // se var = passDigits ja leu os digitos da senha

            inicializaSD();

            myFile = SD.open("bdSirLab.txt");           // Abre o arquivo bdSirLab.txt

            if (myFile) {

              Serial.println("Arquivo bdSirLab.txt aberto com sucesso !");
              Serial.println(" ");

              numReg = 0;
              int cont = 1;                                  // conta os caracteres lidos para separar a senha (4 digitos) do nome do usuario (n digitos)
              char diGito;                                   // cada digito lido do arquivo
              bool achou = false;
              passLida = "";
              usuS = "";

              while (myFile.available() && (!achou)) {       // Le o arquivo até o final

                diGito = myFile.read();

                if (char(diGito) != 10) {           // Se o digito for 10 significa que é o retorno de linha do arquivo

                  if (cont <= passDigits) {
                    passLida += char(diGito);
                    cont++;

                  } else {
                    usuS += diGito;

                  }
                } else {
                  cont = 1;
                  numReg++;

                  // ======================== VERIFICAÇÃO SE A PASSWORD DIGITADA COINCIDE COM ALGUMA CADASTRADA ===========

                  if (var == passDigits) {                                  // testa se já foram digitados todos os caracteres da senha
                    bool passOK = false;

                    passDigitada = conv(a) + conv(b) + conv(c) + conv(d);   // monta a password digitada

                    if ((passDigitada == passLida) && (!achou)) {           // testa se a password digitada coincide com alguma cadastrada
                      passOK = true;
#if modoDebug == 1
                      Serial.print("Registro: ");
                      Serial.print(numReg);
                      Serial.print(" passLida: ");
                      Serial.print(passLida);
                      Serial.print(" ");
                      Serial.print(dataHora);
                      Serial.print(" Usuário: ");
                      Serial.println(usuS);
                      Serial.print("     ============ > PASSWORD VALIDA !");
#endif
                      Serial.flush();
                      achou = true;

                      myFile.close();                                              // <----------------------- FECHA O ARQUIVO
#if modoDebug == 1
                      Serial.println("FECHOU O ARQUIVO bdSirLab.txt !!!");
                      Serial.println("");
#endif

                      //******************* SE A passLida ESTÁ CORRETA, ABRE O ARQUIVO log.txt PARA ESCRITA *********************

                      myFile = SD.open("log.txt", FILE_WRITE);                  // Abre o arquivo log.txt para escrita
                      if (myFile) {

#if modoDebug == 1
                        Serial.println("Arquivo log.txt aberto com sucesso !");
#endif
                        dataHora += usuS;                                       // Concatena a hora com o nome do usuário

                        myFile.print(dataHora);                                 // Escreve no arquivo log.txt
#if modoDebug == 2
                        Serial.print("Gravou no log.txt: ");
                        piscaLedVm(2, 200);                               // Sinaliza que foi gravado no arq. log.txt
                        Serial.println(dataHora);
#endif
                      } else {
#if modoDebug == 1
                        Serial.println("Erro ao abrir o arquivo log.txt");         // Se o arquivo nao abrir exibe mensagem
#endif
                        piscaLedVm(10, 10);                                        // para saber se houve erro ao abrir o arquivo
                      }

                      myFile.close();                                    // <----------------------- FECHA O ARQUIVO log.txt
#if modoDebug == 1
                      Serial.println("FECHOU O ARQUIVO log.txt !!!");
#endif

                      // ********************************************************************************

                    } else {
                      passLida = "";
                      usuS = "";
                    }
                  }

                  // ======================================================================================================


                }                                       // fim do if(char(diGito)
              }                // fim do while
            } else {
#if modoDebug == 1
              Serial.println("Erro ao abrir o arquivo bdSirLab.txt");    // Se o arquivo nao abrir exibe mensagem
#endif
              piscaLedVm(10, 10);                                        // para saber se houve erro ao abrir o arquivo
            }                                                            // fim do if (myFile)

          }     // fim do if(var == passDigits) E fim de leitura e montagem da password e do nome do usuario


          if (passDigitada == passLida) {                                  // confirma se a password digitada é válida
            var = 0;
            digitalWrite(pinRele, LOW);             // comanda o rele de abertura da porta
            digitalWrite(pinLedVd, HIGH);            // acende o led verde
            digitalWrite(pinLedVm, LOW);             // apaga o led vermelho
            digitalWrite(pinBuzzer, HIGH);           // confirmação sonora de abertura da porta
            delay(200);
            digitalWrite(pinRele, HIGH);              // desativa o rele de abertura
            digitalWrite(pinLedVd, LOW);             // apaga o led verde
            digitalWrite(pinLedVm, HIGH);            // acende o led vermelho
            digitalWrite(pinBuzzer, LOW);            // desativa o buzzer
          } else {
            piscaLedVm(5, 20);                        // se password invalida pisca o led vermelho e aciona buzzer 5 vezes
          }
          var = 0;
      }

      // -------------------------   fim do switch case   -------------------------------
    }                                               // fim do if (k2 != -6) -> (* anula)
    else {                                          // se digitou * anula
      var = 0;
      piscaLedVm(5, 20);                            // se teclou anular pisca o led vermelho 5 vezes
    }
  }                                                 // fim do if para testar se foi teclado algo

  digitalWrite(pinRele, HIGH);                      // libera o rele
  //limpaArray();                                     // limpa senha e nome para próxima entrada

}     // *************************** FIM DO void loop() ***************************

void piscaLedVm(int numPisca, int tempo) {          // pisca o led vermelho junto com o acionamento do buzzer
  for (int n = 1; n <= numPisca; n++) {
    digitalWrite(pinLedVm, LOW);
    digitalWrite(pinBuzzer, HIGH);
    delay(tempo);
    digitalWrite(pinLedVm, HIGH);
    digitalWrite(pinBuzzer, LOW);
    delay(tempo * 2);
  }
}

void testaLeds() {
  digitalWrite(pinLedVm, HIGH);
  digitalWrite(pinBuzzer, HIGH);
  delay(300);
  digitalWrite(pinLedVm, LOW);
  digitalWrite(pinBuzzer, LOW);
  delay(300);
  digitalWrite(pinLedVd, HIGH);
  digitalWrite(pinBuzzer, HIGH);
  delay(300);
  digitalWrite(pinLedVd, LOW);
  digitalWrite(pinBuzzer, LOW);
  delay(2000);
}
void inicializaSD() {                                             // inicializa o cartão SD
#if modoDebug == 2
  Serial.print("Inicializando o Micro SD ...........      ");
#endif
  if (!SD.begin(4)) {
#if modoDebug == 2
    Serial.print("INICIALIZAÇÃO DO MICRO SD FALHOU !!!");
#endif
    piscaLedVm(3, 500);
    while (1);
  }
#if modoDebug == 2
  Serial.println(" Inicializacao OK.");
#endif
}

String conv(int tecla) {                          // converte os caracteres tipo char para String
  switch (tecla) {
    case 1:
      return ("1");
      break;
    case 2:
      return ("2");
      break;
    case 3:
      return ("3");
      break;
    case 4:
      return ("4");
      break;
    case 5:
      return ("5");
      break;
    case 6:
      return ("6");
      break;
    case 7:
      return ("7");
      break;
    case 8:
      return ("8");
      break;
    case 9:
      return ("9");
      break;
    case 0:
      return ("0");
      break;
    case 17:
      return ("A");
      break;
    case 18:
      return ("B");
      break;
    case 19:
      return ("C");
      break;
    case 20:
      return ("D");
      break;
    case -6:
      return ("*");
      break;
    case -13:
      return ("#");
      break;
  }
}

void mostraData() {

#if modoDebug == 2
  Serial.print("Data: ");
  Serial.print(rtc.getDateStr());
  Serial.print(" - ");
  Serial.println(rtc.getTimeStr());
  delay(1000);
#endif
}
