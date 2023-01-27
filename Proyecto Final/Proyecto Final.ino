/**
 * @mainpage Proyecto Final Arquitectura Computacional
 *
 * @author Jojan Esteban Serna Serna
 * @author Santiago Agredo Vallejo
 * @date 26 de Enero de 2023
 *
 * @section descripcion Descripción
 * Este proyecto está diseñado para utilizar sensores para monitorear la temperatura y utilizar máquinas de Mealy y tareas asíncronas para realizar funciones de seguridad, configuración y monitoreo en un sistema basado en Arduino.
 *
 * @section objetivos Objetivos
 * - Utilizar sensores para monitorear la temperatura
 * - Implementar máquinas de Mealy para controlar el flujo de datos
 * - Implementar tareas asíncronas para manejar tareas de seguridad, configuración y monitoreo en paralelo
 * - Crear una interfaz fácil de usar para configurar y monitorear el sistema
 */

#include <LiquidCrystal.h>
#include <LiquidMenu.h>
#include <Keypad.h>
#include <AsyncTaskLib.h>
#include <DHTStable.h>
#include <Config.h>
#include <EasyBuzzer.h>
#include <EEPROM.h>
#include "Configuracion.h"
#define WOKWI false
#if WOKWI
#include "StateMachineLib.h"
#else
#include <StateMachineLib.h>
#endif


/**
 * @defgroup sensor_temperatura Sensor de Temperatura
 * @brief Sección de código encargada de la inicialización y uso del sensor de temperatura DHT mediante la librería DHTStable.
 * 
 * La sección de código marcada con el pragma "region sensor_temperatura" contiene el código relacionado con el sensor de temperatura utilizado en el proyecto. En este caso, se está utilizando la librería DHTStable para interactuar con el sensor DHT.
 * La variable DHT se utiliza para crear un objeto de la clase DHTStable, el cual se utiliza para recoger y procesar los datos del sensor.
 * La sección termina con el pragma "endregion" indicando el final de esta sección de código.
 *
 * 
 */
#pragma region sensor_temperatura
DHTStable DHT;
#pragma endregion
/**
 * @defgroup configuracion_teclado Configuración del Teclado
 * @brief Sección de código encargada de configurar y utilizar un teclado matricial de 4x4 conectado mediante pines de entrada y salida.
 * 
 * En esta sección se definen las constantes ROWS y COLS, que indican el número de filas y columnas del teclado. También se declara una matriz "keys" que contiene los caracteres asociados a cada tecla del teclado.
 * Se declaran dos arreglos "rowPins" y "colPins" que contienen los pines de entrada y salida respectivamente, conectados al teclado.
 * Se crea un objeto "keypad" de la clase "Keypad" utilizando la función "makeKeymap" para asociar la matriz "keys" con los pines de entrada y salida.
 * La sección termina con el pragma "endregion" indicando el final de esta sección de código.
 *
 * 
 */
#pragma region configuracion_teclado
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 39, 41, 43, 45 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 47, 49, 51, 53 };  //connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
#pragma endregion
/**
 * @defgroup configuracion_lcd Configuración del LCD
 * @brief Sección de código encargada de configurar y utilizar una pantalla LCD conectada mediante I2C o pines de entrada y salida.
 * 
 * En esta sección se utilizan las directivas de preprocesador "#if defined" y "#elif defined" para determinar si se está utilizando la librería "LiquidCrystal_I2C" o "LiquidCrystal" para interactuar con el LCD.
 * En caso de utilizar la librería "LiquidCrystal_I2C", se crea un objeto "lcd" de la clase "LiquidCrystal_I2C" con una dirección I2C específica y un tamaño de 16 columnas y 2 filas.
 * En caso de utilizar la librería "LiquidCrystal", se crea un objeto "lcd" de la clase "LiquidCrystal" utilizando pines específicos para las entradas RS, EN, D4, D5, D6 y D7.
 * La sección termina con el pragma "endregion" indicando el final de esta sección de código.
 *
 * 
 */
#pragma region configuracion_lcd
#if defined(LiquidCrystal_I2C_h)
LiquidCrystal_I2C lcd(0x27, 16, 2);
#elif defined(LiquidCrystal_h)
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);
#endif
#pragma endregion

/**
 * @defgroup configuracion_maquina_estados_y_tareas Configuración de la Máquina de Estados y las Tareas Asíncronas
 * @brief Sección de código encargada de configurar y utilizar una máquina de estados y tareas asíncronas para controlar el comportamiento del sistema.
 * 
 * En esta sección se configura y se utiliza una máquina de estados y tareas asíncronas para controlar el comportamiento del sistema.
 * La sección termina con el pragma "endregion" indicando el final de esta sección de código.
 *
 * 
 */
#pragma region configuracion_maquina_estados_y_tareas
/**
 * @struct Estado
 * @brief Enumeración para los estados en los que se puede encontrar el sistema
 *
 * Esta enumeración representa los diferentes estados en los que se puede encontrar el sistema.
 * - Inicio: Estado inicial del sistema, en el que se realizan las configuraciones necesarias antes de comenzar a funcionar y se solicita la contraseña
 * - Configuracion: Estado en el que se permite al usuario configurar las opciones del sistema.
 * - Monitoreo: Estado en el que el sistema se encarga de monitorear continuamente los sensores.
 * - Alarma: Estado en el que se activa la alarma debido a una condición determinada.
 */
enum Estado {
  Inicio,
  Configuracion,
  Monitoreo,
  Alarma
};
/**
 * @struct Entrada
 * @brief Enumeración para las entradas a la máquina de estados
 *
 * Esta enumeración representa las diferentes entradas a la máquina de estados.
 * - Desconocida: Entrada no reconocida.
 * - Configurar: Entrada para cambiar al estado de configuración.
 * - Monitorear: Entrada para cambiar al estado de monitoreo.
 * - TemperaturaCaliente: Entrada que indica que la temperatura ha superado el límite establecido.
 */
enum Entrada {
  Desconocida,
  Configurar,
  Monitorear,
  TemperaturaCaliente,
};


/**
 * @class MaquinaDeEstados
 * @brief Clase que implementa una máquina de estados para controlar el funcionamiento del sistema
 *
 * Esta clase hereda de la clase StateMachine y se utiliza para controlar el funcionamiento del sistema a través de una máquina de estados.
 * 
 * La clase tiene un atributo privado de tipo Entrada que representa la entrada actual a la máquina de estados.
 * 
 * Contiene métodos públicos para establecer y obtener la entrada actual, y un constructor para inicializar la máquina de estados.
 */
class MaquinaDeEstados : public StateMachine {
private:
  Entrada entradaActual;
public:


  MaquinaDeEstados()
    : StateMachine(4, 6) {
  }

  void setEntradaActual(Entrada entrada) {
    entradaActual = entrada;
    Update();
  }

  Entrada getEntradaActual() {
    return entradaActual;
  }
};
/**
 * @var maquinaEstados
 * @brief Objeto de la clase MaquinaDeEstados
 *
 * Este objeto se utiliza para controlar el funcionamiento del sistema a través de la máquina de estados.
 */
MaquinaDeEstados maquinaEstados;
/**
 * @defgroup tareas Tareas
 * @brief Sección del código donde se encuentran las tareas asíncronas del sistema
 *
 * En esta sección del código se encuentran las funciones y variables relacionadas con las tareas asíncronas del sistema, 
 * tales como la lectura de sensores, actualización de pantallas y manejo de alarmas.
 */
#pragma region tareas
/**
 * @defgroup seguridad Seguridad
 * @brief Sección del código donde se encuentran las funciones de seguridad
 *
 * En esta sección del código se encuentran las funciones y variables relacionadas con la seguridad del sistema, 
 * tales como el manejo de contraseñas y la autenticación del usuario
 */
#pragma region seguridad
bool flgEsperar = false;
bool flgQuedanIntentos = true;
bool flgPuedeLeer = true;
bool flgPasswordcorrecto = false;
bool flgPasswordIngresado = false;
bool flgFirstCharacter = true;
String contrasenia_leida = "";
signed char conteoCaracteres = 0;
String contrasenia = "12345";
signed char intentos = 0;
long elapsedtime = 0;
/**
* @brief Genera un color a partir de los valores RGB dados.
* @param red Valor para el color rojo (0-255)
* @param green Valor para el color verde (0-255)
* @param blue Valor para el color azul (0-255)
*/
void color(unsigned char red, unsigned char green, unsigned char blue)  // the color generating function
{

  analogWrite(PIN_LED_RED, red);
  analogWrite(PIN_LED_BLUE, blue);
  analogWrite(PIN_LED_GREEN, green);
};
/**
 * @brief Tarea asíncrona para esperar 5 segundos
 * @details Esta tarea se encarga de mostrar un mensaje en una pantalla LCD indicando al usuario que debe esperar 5 segundos antes de intentar nuevamente. 
 * Se utiliza un bucle for para contar los segundos y mostrarlos en la pantalla.
 * Si la variable flgEsperar es verdadera, se muestra el mensaje en la pantalla y se espera 1 segundo antes de continuar con la siguiente iteración del bucle.
 * Al finalizar la espera, se limpia la pantalla y se muestra el mensaje "Sigue intentando", se detiene la señal del buzzer, se reinician las banderas y se reinicia el contador de intentos.
 */
AsyncTask tskAwaitFiveSeconds(300, true, []() {
  if (flgEsperar) {
    for (int i = 5; i > 0; i--) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Espere ");
      lcd.print(i);
      lcd.println(" Secs");
      delay(1000);
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sigue intentando");
    EasyBuzzer.stopBeep();
    flgEsperar = false;
    flgPuedeLeer = true;
    flgQuedanIntentos = true;
    flgPasswordcorrecto = false;
    flgPasswordIngresado = false;
    intentos = 0;
  }
});
/**
 * @brief Tarea asíncrona para esperar 10 segundos
 * @details Esta tarea se encarga de esperar 10 segundos antes de mostrar un mensaje en una pantalla LCD indicando al usuario que debe intentar nuevamente.
 * Se utiliza un delay de 10 segundos(El delay de la tarea) para detener la ejecución del código durante ese tiempo.
 *  Al finalizar la espera, se limpia la pantalla, se coloca el cursor en la posición 0,0, se muestra el mensaje "time out: try again", se reinician las banderas "flgFirstCharacter" y "flgPasswordIngresado" y se establece flgPasswordcorrecto = false.
 */
AsyncTask tskAwaitTenSeconds(10000, false, []() {
  //verificarContrasenia();
  flgFirstCharacter = true;
  flgPasswordIngresado = true;
  flgPasswordcorrecto = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("time out: try again");
});
/**
 * @brief Función para verificar contraseña
 * @details Esta función se encarga de comparar una contraseña almacenada previamente con una contraseña leida. 
 * Si las contraseñas coinciden se muestra un mensaje "Contraseña correcta" en la consola serial y en una pantalla LCD y se establece la bandera flgPasswordcorrecto = true
 * Si las contraseñas no coinciden se muestra un mensaje "Contraseña incorrecta" en la consola serial y en una pantalla LCD y se establece la bandera flgPasswordcorrecto = false
 */
void verificarContrasenia() {
  if (contrasenia.equals(contrasenia_leida)) {
    Serial.println("Contraseña correcta");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Correcto");
    flgPasswordcorrecto = true;
  } else {
    Serial.println("Contraseña incorrecta");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Incorrecto");
    flgPasswordcorrecto = false;
  }
}
/*****************************************************************************
*@brief Tarea asíncrona para leer contraseña
*@details Esta tarea asíncrona se encarga de leer caracteres de un teclado y mostrarlos en una pantalla LCD como asteriscos.
*Si se presiona la tecla '*' o se han ingresado 8 caracteres, se llama a la función verificarContrasenia() para comparar la contraseña ingresada con una almacenada previamente.
*También se manejan banderas flgPuedeLeer, flgQuedanIntentos, flgFirstCharacter y flgPasswordIngresado para controlar el estado de la tarea y evitar ingresos no deseados.
*/
AsyncTask tskLeerPassword(100, true, []() {
  if (flgPuedeLeer && flgQuedanIntentos) {
    char key = keypad.getKey();

    if (key == '*' || conteoCaracteres == 8) {
      verificarContrasenia();
      tskAwaitTenSeconds.Stop();
      flgFirstCharacter = true;
      flgPasswordIngresado = true;
    }

    if (key != '*' && key != NO_KEY) {
      if (flgFirstCharacter) {
        tskAwaitTenSeconds.Start();
        flgFirstCharacter = false;
      }
      tskAwaitTenSeconds.Reset();
      contrasenia_leida += key;
      conteoCaracteres++;
      Serial.print("Contraseña leida: ");
      Serial.println(contrasenia_leida);
      char tempBuffer[9] = "";
      memset(tempBuffer, '*', conteoCaracteres);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(tempBuffer);
    }
  }
});
/**
* @brief Tarea asíncrona para tomar decision de una contraseña

* @details  tskDecisionPassword es una tarea asíncrona que se encarga de tomar
*         una decisión basada en el resultado de la contraseña ingresada
*         previamente.
*
*
*/
AsyncTask tskDecisionPassword(200, true, []() {
  if (flgPasswordIngresado && !flgEsperar) {
    contrasenia_leida = "";
    conteoCaracteres = 0;
    flgPasswordIngresado = false;
    if (flgPasswordcorrecto) {
      color(0, 255, 0);
      Serial.println("LED VERDE");
      maquinaEstados.setEntradaActual(Entrada::Configurar);
    } else {
      color(0, 0, 255);
      Serial.println("LED AZUL");
      intentos++;
    }
    if (intentos > 3) {
      flgQuedanIntentos = false;
      flgPuedeLeer = false;
      flgEsperar = true;
      color(255, 0, 0);
      Serial.println("LED ROJO");
      EasyBuzzer.singleBeep(
        300,  // Frequency in hertz(HZ).
        500   // Duration of the beep in milliseconds(ms).
      );
    }
  }
});
/**
*@brief AsyncTask que se encarga de manejar la seguridad del sistema
*@details En este AsyncTask se encuentra el loop principal de la seguridad del sistema.
*En el se inicializan los pines necesarios para el funcionamiento del sistema, 
*se inician las tareas necesarias para el funcionamiento del sistema y se actualizan los estados de las tareas y componentes del sistema.
*Dentro del loop se encuentra la llamada a las funciones de actualizacion de las tareas de lectura de contraseña, espera de 10 segundos, 
*toma de decisiones y espera de 5 segundos. También se encuentra la llamada a la función de actualización del buzzer pasivo.
*El loop se encuentra dentro de un while que se mantiene activo mientras el estado de la máquina de estados sea el de Inicio.
*/
AsyncTask tskSeguridad(100, []() {
  Serial.println("Entre hasta aqui");
  auto setup = []() {
    pinMode(PIN_BUZZER_PASIVO, OUTPUT);  // pin 8 como salida
    Serial.begin(9600);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    EasyBuzzer.setPin(PIN_BUZZER_PASIVO);
    tskLeerPassword.Start();
    tskDecisionPassword.Start();
    tskAwaitFiveSeconds.Start();
    lcd.println("Ingrese Contra");
  };

  auto loop = []() {
    tskLeerPassword.Update();
    tskAwaitTenSeconds.Update();
    tskDecisionPassword.Update();
    tskAwaitFiveSeconds.Update();
    EasyBuzzer.update();
  };

  setup();
  while (maquinaEstados.GetState() == Estado::Inicio) {
    Serial.println("Loop Seguridad");

    loop();
  }
});

#pragma endregion
/**
*@defgroup menu Menu
*@brief Sección del código donde se encuentra el menú de opciones
*En esta sección del código se encuentran las funciones y variables relacionadas con el menú de opciones del sistema,
*permitiendo al usuario acceder a diferentes funcionalidades y configuraciones del mismo.
*/
#pragma region menu
/**
*@struct Umbrales
*@brief Estructura que contiene los umbrales de control del sistema
*Esta estructura contiene los siguientes atributos:
*checkKey: representa una clave de verificación que se usa para evitar leer basura de la memoria EEPROM en el primer inicio del sistema
*umbrTempHigh: representa el umbral alto de temperatura
*umbrTempLow: representa el umbral bajo de temperatura
*umbrLuzHigh: representa el umbral alto de luz
*umbrLuzLow: representa el umbral bajo de luz
*/
typedef struct Umbrales {
  int checkKey;
  int umbrTempHigh;
  int umbrTempLow;
  int umbrLuzHigh;
  int umbrLuzLow;
} Umbrales;

Umbrales umbralConfig;
Umbrales umbralBaseConfig = Umbrales{ 192837465, DEFAULT_TEMPHIGH, DEFAULT_TEMPLOW, DEFAULT_LUZHIGH, DEFAULT_LUZLOW };
int eepromBaseAddres = 0;
int readNumber();
void editar_valor(String titulo, int *varimp, bool (*isInRangeFunction)(int, int *));
bool isInTempRange(int number, int *varimp);
bool isInLightRange(int number, int *varimp);
void umbTempHighFunc();
void umbTempLowFunc();
void umbLuzHighFunc();
void umbLuzLowFunc();

#pragma region Screens
#if WOKWI
char messages[5][16] = { { "1.UmbTempHigh" }, { "2.UmbTempLow" }, { "3.UmbLuzHigh" }, { "4.UmbLuzLow" }, { "5.Reset" } };
#else
char *messages[5] = { "1.UmbTempHigh", "2.UmbTempLow", "3.UmbLuzHigh", "4.UmbLuzLow", "5.Reset" };
#endif
LiquidScreen *lastScreen = nullptr;

LiquidLine screen_1_line_1(0, 0, messages[0]);
LiquidLine screen_1_line_2(0, 1, messages[1]);
LiquidScreen screen_1(screen_1_line_1, screen_1_line_2);

LiquidLine screen_2_line_1(0, 0, messages[1]);
LiquidLine screen_2_line_2(0, 1, messages[2]);
LiquidScreen screen_2(screen_2_line_1, screen_2_line_2);

LiquidLine screen_3_line_1(0, 0, messages[2]);
LiquidLine screen_3_line_2(0, 1, messages[3]);
LiquidScreen screen_3(screen_3_line_1, screen_3_line_2);

LiquidLine screen_4_line_1(0, 0, messages[3]);
LiquidLine screen_4_line_2(0, 1, messages[4]);
LiquidScreen screen_4(screen_4_line_1, screen_4_line_2);

LiquidLine screen_5_line_1(0, 0, "");
LiquidLine screen_5_line_2(0, 1, "");
LiquidScreen screen_5(screen_5_line_1, screen_5_line_2);

LiquidMenu menu(lcd, screen_1, screen_2, screen_3, screen_4);

#pragma endregion
/**
* @brief Esta funcion lee un numero ingresado desde el teclado y lo devuelve como un entero
* @param None
* @return int numero ingresado desde el teclado, o un codigo que indica error
*/
int readNumber() {
  lcd.setCursor(0, 1);

  lcd.print("                ");
  lcd.setCursor(0, 1);

  String strNumber = "";
  char readedChar;
  int checkPointTime = millis();
  int elapsedTime = millis() - checkPointTime;
  while (elapsedTime <= 5000) {
    char key = keypad.getKey();
    if (key) {
      checkPointTime = millis();

      if (key == '*') {
        break;  // clear input
      } else if (isAlpha(key)) {
        continue;
      } else if (isDigit(key)) {  // only act on numeric keys
        strNumber += key;         // append new character to input string
        lcd.print(key);
      }
    }
    elapsedTime = millis() - checkPointTime;
  }
  if (elapsedTime >= 5000) {
    return 19283747;
  }
  return strNumber.toInt();
}
/**
* @brief Evalua si un numero se encuentra en el rango de temperatura especificado
* @param number numero a evaluar
* @param varimp Puntero al limite inferior o superior del rango de temperatura
* @return true si el numero se encuentra en el rango, false en caso contrario
*/
bool isInTempRange(int number, int *varimp) {
  return ((varimp == &umbralConfig.umbrTempLow && number < umbralConfig.umbrTempHigh || varimp == &umbralConfig.umbrTempHigh && number > umbralConfig.umbrTempLow) && number <= MAX_TEMP);
}
/**
* @brief Comprueba si un número está dentro del rango especificado de un umbral de luz.
*
* @param number Número a comprobar
* @param varimp Puntero al umbral a comparar. Puede ser el umbral alto o bajo.
*
* @return true si el número está dentro del rango especificado, false en caso contrario.
*/
bool isInLightRange(int number, int *varimp) {
  return ((varimp == &umbralConfig.umbrLuzLow && number < umbralConfig.umbrLuzHigh || varimp == &umbralConfig.umbrLuzHigh && number > umbralConfig.umbrLuzLow) && number <= MAX_LIGTH);
}

/**
 * @brief Permite al usuario editar un valor específico en pantalla
 * 
 * @param titulo El título a mostrar en la pantalla para indicar cual valor se está editando
 * @param varimp Puntero al valor a editar
 * @param isInRangeFunction Puntero a función para verificar que el valor ingresado está en el rango permitido
 */
void editar_valor(String titulo, int *varimp, bool (*isInRangeFunction)(int, int *)) {
  menu.change_screen(&screen_5);
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(titulo);
  lcd.setCursor(0, 1);
  lcd.print(*varimp);
  lcd.print(" \"*\" to edit");
  char pressedKey;
  int checkPointTime = millis();
  int elapsedTime = millis() - checkPointTime;
  while ((pressedKey = keypad.getKey()) != '*' && pressedKey == NO_KEY && pressedKey != '#' && (elapsedTime = millis() - checkPointTime) <= 5000 || isAlphaNumeric(pressedKey)) {
  }
  if (pressedKey == '#' || elapsedTime > 5000) {
    menu.change_screen(lastScreen);
    return;
  }
  int number = readNumber();
  if (number != 19283747 && isInRangeFunction(number, varimp)) {
    *varimp = number;
    EEPROM.put(eepromBaseAddres, umbralConfig);
    menu.change_screen(lastScreen);
    return;
  }

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  checkPointTime = millis();
  elapsedTime = millis() - checkPointTime;
  lcd.print("Error press \"*\"");
  while ((pressedKey = keypad.getKey()) != '*' && pressedKey == NO_KEY && (elapsedTime = millis() - checkPointTime) <= 5000 || pressedKey == '#' || isAlphaNumeric(pressedKey)) {
  }

  menu.change_screen(lastScreen);
}
/**
 * @brief Función para editar el valor de UmbTempHigh
 * 
 * @param[in] umbralConfig Estructura que contiene la configuración de los umbrales.
 * @param[in] umbrTempHigh Puntero al valor del umbral alto de temperatura en la configuración.
 * @param[in] isInTempRange Puntero a la función utilizada para verificar si el valor ingresado se encuentra en el rango permitido.
 */
void umbTempHighFunc() {
  editar_valor("UmbTempHigh", &umbralConfig.umbrTempHigh, isInTempRange);
};
/**
 * @brief Esta función se encarga de editar el valor del umbral bajo de temperatura en la configuración de UmbTempLow. 
 * Utiliza la función "editar_valor" para realizar esta tarea y verifica si el valor ingresado se encuentra en el rango de temperatura permitido mediante la función "isInTempRange".
 *
 * @param[in] umbralConfig Estructura que contiene la configuración de los umbrales.
 * @param[in] umbrTempLow Puntero al valor del umbral bajo de temperatura en la configuración.
 * @param[in] isInTempRange Puntero a la función utilizada para verificar si el valor ingresado se encuentra en el rango permitido.
 */
void umbTempLowFunc() {
  editar_valor("UmbTempLow", &umbralConfig.umbrTempLow, isInTempRange);
};
/**
 * @brief Esta función se encarga de editar el valor del umbral bajo de temperatura en la configuración de UmbTempLow. 
 * Utiliza la función "editar_valor" para realizar esta tarea y verifica si el valor ingresado se encuentra en el rango de temperatura permitido mediante la función "isInTempRange".
 *
 * @param[in] umbralConfig Estructura que contiene la configuración de los umbrales.
 * @param[in] UmbLuzHigh Puntero al valor del umbral alto de luz en la configuración.
 * @param[in] isInTempRange Puntero a la función utilizada para verificar si el valor ingresado se encuentra en el rango permitido.
 */
void umbLuzHighFunc() {
  editar_valor("UmbLuzHigh", &umbralConfig.umbrLuzHigh, isInLightRange);
};
/**
 * @brief Esta función se encarga de editar el valor del umbral bajo de temperatura en la configuración de UmbTempLow. 
 * Utiliza la función "editar_valor" para realizar esta tarea y verifica si el valor ingresado se encuentra en el rango de temperatura permitido mediante la función "isInTempRange".
 *
 * @param[in] umbralConfig Estructura que contiene la configuración de los umbrales.
 * @param[in] UmbLuzLow Puntero al valor del umbral bajo de luz en la configuración.
 * @param[in] isInTempRange Puntero a la función utilizada para verificar si el valor ingresado se encuentra en el rango permitido.
 */
void umbLuzLowFunc() {
  editar_valor("UmbLuzLow", &umbralConfig.umbrLuzLow, isInLightRange);
};

 /**
 *   @brief AsyncTask para configurar los umbrales de temperatura y luz.
 *   @details El objeto AsyncTask "tskConfiguracion" se encarga de la configuración de los umbrales de temperatura y luz.
 *   Al iniciar, se le proporciona un tiempo de ejecución de 100ms y una función lambda que contiene el código de configuración.
 *   En la función de configuración, se carga la configuración actual desde la memoria EEPROM y se verifica si es válida.
 *   Si no es válida, se establece la configuración predeterminada.
 *   Luego se configura el modo de los pines correspondientes a los LEDs, se asignan funciones a los elementos de pantalla y se actualiza el menú.
 *   En la función de bucle, se verifica si se ha presionado alguna tecla en el teclado y se realizan acciones correspondientes como cambiar de pantalla,
 *   activar o desactivar el enfoque en el menú (la flecha), y llamar a funciones asociadas a cada elemento de pantalla.
 *   El bucle se ejecutará mientras el estado de la máquina de estados sea configuración.
*/
AsyncTask tskConfiguracion(100, []() {
  auto setup = []() {
    EEPROM.get(eepromBaseAddres, umbralConfig);
    if (umbralConfig.checkKey != umbralBaseConfig.checkKey) {
      umbralConfig = umbralBaseConfig;
      EEPROM.put(eepromBaseAddres, umbralConfig);
    }
    menu.add_screen(screen_5);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);

    // screen_2_line_1.attach_function(1,)
    screen_1_line_1.attach_function(1, umbTempHighFunc);
    screen_1_line_2.attach_function(1, umbTempLowFunc);

    screen_2_line_1.attach_function(1, umbTempLowFunc);
    screen_2_line_2.attach_function(1, umbLuzHighFunc);

    screen_3_line_1.attach_function(1, umbLuzHighFunc);
    screen_3_line_2.attach_function(1, umbLuzLowFunc);

    screen_4_line_1.attach_function(1, umbLuzLowFunc);
    screen_4_line_2.attach_function(1, []() {
      menu.change_screen(&screen_5);

      lcd.setCursor(0, 0);
      lcd.print("                ");

      lcd.setCursor(0, 0);
      lcd.print("\"*\" to confirm ");
      lcd.setCursor(0, 1);
      lcd.print("\"#\" to cancel  ");
      char pressedKey;
      int checkPointTime = millis();
      int elapsedTime = millis() - checkPointTime;
      while ((pressedKey = keypad.getKey()) != '*' && pressedKey == NO_KEY && pressedKey != '#' && (elapsedTime = millis() - checkPointTime) <= 5000 || isAlphaNumeric(pressedKey)) {
      }
      if (pressedKey == '#' || elapsedTime > 5000) {
        menu.change_screen(lastScreen);
        return;
      }
      umbralConfig.umbrTempHigh = DEFAULT_TEMPHIGH;
      umbralConfig.umbrTempLow = DEFAULT_TEMPLOW;      
      EEPROM.put(eepromBaseAddres, umbralConfig);
      menu.change_screen(lastScreen);
    });
    menu.update();
  };

  auto loop = []() {
    char key = keypad.getKey();
    if (key == 'A') {

      if (menu.get_currentScreen() != &screen_1) {

        menu.previous_screen();
      }
    } else if (key == 'D') {

      if (menu.get_currentScreen() != &screen_4) {
        menu.next_screen();
      }
    } else if (key == '#') {
      menu.switch_focus();
    } else if (key == '*') {
      lastScreen = menu.get_currentScreen();
      menu.call_function(1);
      // lcd.print(readNumber());
    }
  };

  setup();
  while (maquinaEstados.GetState() == Estado::Configuracion) {
    loop();
  }
});

#pragma endregion
/**
*@defgroup monitoreo Monitoreo
*@brief Sección del código donde se encuentra el codigo del monitoreo de temperatura y humedad
*En esta sección del código se encuentran las funciones y variables relacionadas con el monitoreo,
*contiene tareas y funciones relacionadas a esto
*/
#pragma region monitoreo
/**
 * @brief Verifica los errores devueltos por la librería DHTStable.
 * 
 * La función verificarErrores() recibe como parámetro un entero "chk" que representa el código de error devuelto por la librería DHTStable.
 * Se utiliza un switch-case para analizar el código de error y imprimir en el monitor serie un mensaje descriptivo del error.
 * Si el código de error es igual a DHTLIB_OK, se imprime "OK".
 * Si el código de error es igual a DHTLIB_ERROR_CHECKSUM, se imprime "Checksum error".
 * Si el código de error es igual a DHTLIB_ERROR_TIMEOUT, se imprime "Time out error".
 * Si el código de error es desconocido, se imprime "Unknown error".
 * 
 * @param chk Código de error devuelto por la librería DHTStable.
 */
void verificarErrores(int chk) {
  switch (chk) {
    case DHTLIB_OK:
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.print("Time out error,\t");
      break;
    default:
      Serial.print("Unknown error,\t");
      break;
  }
}
float humedadLeida, temperaturaLeida;
/**
* @brief AsyncTask para leer la temperatura desde el sensor DHT
* @details El objeto AsyncTask "tskLeerTemperatura" se encarga de la lectura de la temperatura desde el sensor DHT.
* Al iniciar, se le proporciona un tiempo de ejecución de 2000ms y la opción de ejecutar en modo cíclico.
* En su función lambda se realiza la lectura del sensor utilizando la librería DHTSatble, se verifica si existe algún error en la lectura
* y se almacena el valor leído en la variable "temperaturaLeida". Además, se imprime el valor leído por el puerto serie.
* Esta tarea es utilizada para actualizar periódicamente la lectura de la temperatura en la aplicación.
*/
AsyncTask tskLeerTemperatura(2000, true, []() {
#if WOKWI
  int chk = DHT.read22(PIN_DHT);
#else
    int chk = DHT.read11(PIN_DHT);
#endif
  verificarErrores(chk);
  temperaturaLeida = DHT.getTemperature();
  Serial.print("Temp: ");
  Serial.println(temperaturaLeida);
});
/**
 * @brief AsyncTask para leer la humedad del sensor DHT
 * 
 * @details Este AsyncTask se encarga de leer la humedad del sensor DHT con una frecuencia
 * especificada en el primer argumento (1000ms en este caso). Si el segundo argumento
 * es true, el AsyncTask se repetirá constantemente. La función leerá la humedad del
 * sensor DHT utilizando la librería DHTStable y almacenará el valor en la variable humedadLeida.
 * También imprimirá el valor de humedad en el puerto serie.
 */
AsyncTask tskLeerHumedad(1000, true, []() {
#if WOKWI
  int chk = DHT.read22(PIN_DHT);
#else
    int chk = DHT.read11(PIN_DHT);
#endif
  verificarErrores(chk);
  humedadLeida = DHT.getHumidity();
  Serial.print("Humd: ");
  Serial.println(humedadLeida);
});
/**
 * @brief La tarea AsyncTask tskActualizarDisplay se encarga de actualizar los datos de humedad y temperatura en el display LCD cada 4 segundos.
 *
 * @details La tarea se ejecuta cada 4 segundos y actualiza los datos de humedad y temperatura en el display LCD.
 *
 */
AsyncTask tskActualizarDisplay(4000, true, []() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humedad: ");
  lcd.print(humedadLeida);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperaturaLeida);
});
/**
* @brief Tarea asíncrona para el monitoreo de alguna variable
* @details La tarea asíncrona se encarga de monitorear alguna variable utilizando
* varios subprocesos. Se inicializa con un intervalo de tiempo de 100ms
* y tiene dos funciones anidadas, 'setup' y 'loop'. La función 'setup'
* configura los pines para el uso del buzzer, los leds y arranca las
* tareas de lectura de temperatura, humedad y actualización del display.
* La función 'loop' se encarga de actualizar las tareas mencionadas
* anteriormente y de cambiar el color de los leds dependiendo del valor
* de la temperatura leída. Si la temperatura es mayor al umbral de temperatura
* alta, el led rojo se encenderá y se cambiara el estado de la maquina. Si es menor al umbral de temperatura baja,
* el led azul se encenderá. Si se encuentra entre ambos umbrales, el led verde
* se encenderá.
* El bucle principal de esta tarea se ejecuta mientras el estado de la máquina de
* estados sea "Monitorear".
*/
AsyncTask tskMonitoreo(100, []() {
  auto setup = []() {
    pinMode(PIN_BUZZER_PASIVO, OUTPUT);  // pin 51 como salida
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    tskLeerTemperatura.Start();
    tskLeerHumedad.Start();
    tskActualizarDisplay.Start();
  };

  auto loop = []() {
    tskLeerTemperatura.Update();
    tskLeerHumedad.Update();
    tskActualizarDisplay.Update();

    if (temperaturaLeida > umbralConfig.umbrTempHigh) {
      color(255, 0, 0);
      temperaturaLeida = 0;
      maquinaEstados.setEntradaActual(Entrada::TemperaturaCaliente);
    } else if (temperaturaLeida < umbralConfig.umbrTempLow) {
      color(0, 0, 255);
    } else if (temperaturaLeida > umbralConfig.umbrTempLow && temperaturaLeida < umbralConfig.umbrTempHigh) {
      color(0, 255, 0);
    }
  };

  setup();
  while (maquinaEstados.GetState() == Entrada::Monitorear) {
    loop();
  }
});

#pragma endregion
/**
 * @defgroup alarma Alarma
 * @brief Sección del código donde se encuentran las tareas asíncronas que hacen sonar la alarma
 *
 * En esta sección del código se encuentran las funciones y tareas asincronas relacionadas con la reproduccion de una alarma
 */
#pragma region alarma
/**
 * @brief AsyncTask para reproducir una alarma
 * @details tskAlarma es una tarea asíncrona que se encarga de activar el buzzer
 * y mostrar un mensaje en el LCD indicando la cantidad de segundos que
 * quedan para desactivar la alarma. La alarma se activa mientras el estado
 * de la máquina de estados sea Estado::Alarma. 
 *
 */
AsyncTask tskAlarma(100, []() {
  auto setup = []() {
    EasyBuzzer.setPin(PIN_BUZZER_PASIVO);
    EasyBuzzer.singleBeep(
      300,  // Frequency in hertz(HZ).
      2000  // Duration of the beep in milliseconds(ms).
    );
  };


  auto loop = []() {
    EasyBuzzer.update();
  };
  setup();
  while (maquinaEstados.GetState() == Estado::Alarma) {
    loop();
    lcd.clear();
    lcd.setCursor(0, 0);
    float checkpointTime = millis();
    float elapsedTime = millis() - checkpointTime;
    while (elapsedTime <= 2000) {
      elapsedTime = millis() - checkpointTime;
      float reversedTime = (2000 - elapsedTime) / 1000;
      lcd.clear();
      lcd.print("Espere ");
      lcd.print(reversedTime, 2);
      lcd.println(" Secs");
    }


    EasyBuzzer.stopBeep();
    maquinaEstados.setEntradaActual(Entrada::Monitorear);
  }
});
#pragma endregion
#pragma endregion
/**
*@brief Configuar la maquina de estados
*@details Configura las transiciones y las acciones al entrar y salir de cada estado de la máquina de estado
*/
void configurarMaquinaEstado() {
  //Transiciones
  maquinaEstados.AddTransition(Inicio, Configuracion, []() {
    return maquinaEstados.getEntradaActual() == Configurar;
  });
  maquinaEstados.AddTransition(Configuracion, Monitoreo, []() {
    return maquinaEstados.getEntradaActual() == Monitorear;
  });
  maquinaEstados.AddTransition(Monitoreo, Configuracion, []() {
    return maquinaEstados.getEntradaActual() == Configurar;
  });
  maquinaEstados.AddTransition(Monitoreo, Alarma, []() {
    return maquinaEstados.getEntradaActual() == TemperaturaCaliente;
  });

  maquinaEstados.AddTransition(Alarma, Monitoreo, []() {
    return maquinaEstados.getEntradaActual() == Monitorear;
  });

  maquinaEstados.AddTransition(Alarma, Configuracion, []() {
    return maquinaEstados.getEntradaActual() == Configurar;
  });

  //Al entrar
  maquinaEstados.SetOnEntering(Inicio, []() {
    Serial.println("Entering Seguridad");
    tskSeguridad.Start();
  });
  maquinaEstados.SetOnEntering(Configuracion, []() {
    Serial.println("Entrando a config");
    color(0, 0, 0);
    tskConfiguracion.Start();
  });
  maquinaEstados.SetOnEntering(Monitoreo, []() {
    Serial.println("Entrando a monitorear");
    color(0, 0, 0);
    tskMonitoreo.Start();
  });
  maquinaEstados.SetOnEntering(Alarma, []() {
    Serial.println("Entrando a la alarma");
    tskAlarma.Start();
  });

  //Al Salir
  maquinaEstados.SetOnLeaving(Inicio, []() {
    tskLeerPassword.Stop();
    tskAwaitTenSeconds.Stop();
    tskDecisionPassword.Stop();
    tskAwaitFiveSeconds.Stop();
    tskSeguridad.Stop();
    Serial.println("La contraseña fue correcta");
  });
  maquinaEstados.SetOnLeaving(Configuracion, []() {
    tskConfiguracion.Stop();
    Serial.println("Saliendo de la configuracion");
  });
  maquinaEstados.SetOnLeaving(Monitoreo, []() {
    tskLeerTemperatura.Stop();
    tskLeerHumedad.Stop();
    tskActualizarDisplay.Stop();
    tskMonitoreo.Stop();
    Serial.println("Saliendo del monitoreo");
  });
  maquinaEstados.SetOnLeaving(Alarma, []() {
    tskAlarma.Stop();
    lcd.clear();
    Serial.println("Saliendo de la alarma");
  });
}

#pragma endregion
/**
* @brief Funcion llamada cuando se presiona el boton.
* @details Cambia el estado de la maquina de estados segun el estado actual.
*/
void botonPresionado() {
  Serial.println("Boton Presionado");
  if (maquinaEstados.GetState() == Estado::Monitoreo) {
    Serial.println("Pasando a configuracion");
    maquinaEstados.setEntradaActual(Entrada::Configurar);
  } else if (maquinaEstados.GetState() == Estado::Configuracion) {
    Serial.println("Pasando a monitorear");
    lcd.clear();
    maquinaEstados.setEntradaActual(Entrada::Monitorear);
  } else if (maquinaEstados.GetState() == Estado::Alarma) {
    Serial.println("Pasando a configuracion desde alarma");
    maquinaEstados.setEntradaActual(Entrada::Configurar);
  }
}
/**
* @brief Configuracion del sistema
* @details se establecen los modos de algunos pines, se configura una interrupción para cuando se presiona un botón y se inicializa una pantalla LCD. También se inicializa 
* una comunicación serie para poder ver mensajes en la consola y se llama a la función llamada "configurarMaquinaEstado" que configura la máquina de estados. 
* Finalmente, se establece el estado inicial de la máquina de estados.
*/
void setup() {
  pinMode(PIN_BOTON, INPUT_PULLUP);
    pinMode(PIN_BUZZER_PASIVO, OUTPUT);

  digitalWrite(PIN_BUZZER_PASIVO,LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_BOTON), botonPresionado, RISING);

#if defined(LiquidCrystal_I2C_h)
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
#elif defined(LiquidCrystal_h)
  lcd.begin(16, 2);
#endif
  Serial.begin(9600);

  Serial.println("Starting State Machine...");
  configurarMaquinaEstado();
  Serial.println("Start Machine Started");

  maquinaEstados.SetState(Inicio, false, true);
}
/**
*
*@brief Función principal de ejecución
*@details Esta función contiene el bucle principal del programa, en el cual se actualiza la máquina de estados y las tareas de seguridad, configuración, monitoreo y alarma.
*/
void loop() {
  maquinaEstados.Update();
  tskSeguridad.Update();
  tskConfiguracion.Update();
  tskMonitoreo.Update();
  tskAlarma.Update();
}
