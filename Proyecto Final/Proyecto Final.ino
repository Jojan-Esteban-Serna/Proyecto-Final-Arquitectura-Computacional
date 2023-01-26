#include <LiquidCrystal.h>
#include <LiquidMenu.h>
#include <Keypad.h>
#include <AsyncTaskLib.h>
#include <DHTStable.h>
#include <Config.h>
#include <EasyBuzzer.h>
#include <EEPROM.h>
#include "Configuracion.h"
#if WOKWI
#include "StateMachineLib.h"
#else
#include <StateMachineLib.h>
#endif
#pragma region declaracion_funciones
void leerContrasenia();
#pragma endregion


#pragma region sensor_temperatura
DHTStable DHT;
#pragma endregion
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

#pragma region configuracion_lcd
#if defined(LiquidCrystal_I2C_h)
LiquidCrystal_I2C lcd(0x27, 16, 2);
#elif defined(LiquidCrystal_h)
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);
#endif
#pragma endregion


#pragma region configuracion_maquina_estados_y_tareas

enum Estado {
  Inicio,
  Configuracion,
  Monitoreo,
  Alarma
};

enum Entrada {
  Desconocida,
  Configurar,
  Monitorear,
  TemperaturaCaliente,
};



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
MaquinaDeEstados maquinaEstados;

#pragma region tareas
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

void color(unsigned char red, unsigned char green, unsigned char blue)  // the color generating function
{

  analogWrite(PIN_LED_RED, red);
  analogWrite(PIN_LED_BLUE, blue);
  analogWrite(PIN_LED_GREEN, green);
};

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

AsyncTask tskAwaitTenSeconds(10000, false, []() {
  //verificarContrasenia();
  flgFirstCharacter = true;
  flgPasswordIngresado = true;
  flgPasswordcorrecto = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("time out: try again");
});

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

AsyncTask tskLeerPassword(100, true, []() {
  if (flgPuedeLeer && flgQuedanIntentos) {
    char key = keypad.getKey();

    if (key == '*'  || conteoCaracteres == 8) {
      verificarContrasenia();
      tskAwaitTenSeconds.Stop();
      flgFirstCharacter = true;
      flgPasswordIngresado = true;
    }

    if (key  != '*' && key != NO_KEY) {
      if (flgFirstCharacter) {
        tskAwaitTenSeconds.Start();
        flgFirstCharacter = false;
      }
      tskAwaitTenSeconds.Reset();
      contrasenia_leida += key ;
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

AsyncTask tskSeguridad(100, []() {
  Serial.println("Entre hasta aqui");
  auto setup = []() {
    pinMode(PIN_BUZZER_PASIVO, OUTPUT);   // pin 8 como salida
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

#pragma region menu
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
//char *messages[5] = { "1.UmbTempHigh", "2.UmbTempLow", "3.UmbLuzHigh", "4.UmbLuzLow", "5.Reset" };
char messages[5][16] = { {"1.UmbTempHigh"}, {"2.UmbTempLow"}, {"3.UmbLuzHigh"}, {"4.UmbLuzLow"}, {"5.Reset"} };

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

bool isInTempRange(int number, int *varimp) {
  return ((varimp == &umbralConfig.umbrTempLow && number < umbralConfig.umbrTempHigh || varimp == &umbralConfig.umbrTempHigh && number > umbralConfig.umbrTempLow) && number <= MAX_TEMP);
}

bool isInLightRange(int number, int *varimp) {
  return ((varimp == &umbralConfig.umbrLuzLow && number < umbralConfig.umbrLuzHigh || varimp == &umbralConfig.umbrLuzHigh && number > umbralConfig.umbrLuzLow) && number <= MAX_LIGTH);
}


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
void umbTempHighFunc() {
  editar_valor("UmbTempHigh", &umbralConfig.umbrTempHigh, isInTempRange);
};

void umbTempLowFunc() {
  editar_valor("UmbTempLow", &umbralConfig.umbrTempLow, isInTempRange);
};

void umbLuzHighFunc() {
  editar_valor("UmbLuzHigh", &umbralConfig.umbrLuzHigh, isInLightRange);
};
void umbLuzLowFunc() {
  editar_valor("UmbLuzLow", &umbralConfig.umbrLuzLow, isInLightRange);
};


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
#pragma region monitoreo
void verificarErrores(int chk)
{
  switch (chk)
  {
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
AsyncTask tskLeerTemperatura(2000, true, []()
{
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

AsyncTask tskLeerHumedad(1000, true, []()
{
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

AsyncTask tskActualizarDisplay(4000, true, []()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humedad: ");
  lcd.print(humedadLeida);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperaturaLeida);
});
AsyncTask tskMonitoreo(100, []() {
  auto setup = []() {
    pinMode(PIN_BUZZER_PASIVO, OUTPUT);   // pin 51 como salida
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

    if (temperaturaLeida > umbralConfig.umbrTempHigh)
    {
      color(255, 0, 0);
      temperaturaLeida = 0;
      maquinaEstados.setEntradaActual(Entrada::TemperaturaCaliente);
    }
    else if (temperaturaLeida < umbralConfig.umbrTempLow)
    {
      color(0, 0, 255);
    }
    else if (temperaturaLeida > umbralConfig.umbrTempLow && temperaturaLeida < umbralConfig.umbrTempHigh)
    {
      color(0, 255, 0);
    }
  };

  setup();
  while (maquinaEstados.GetState() == Entrada::Monitorear) {
    loop();
  }
});

#pragma endregion

#pragma region alarma
AsyncTask tskAlarma(100, []() {
  auto setup = []() {
    EasyBuzzer.setPin(PIN_BUZZER_PASIVO);
    EasyBuzzer.singleBeep(
      300,  // Frequency in hertz(HZ).
      2000   // Duration of the beep in milliseconds(ms).
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
    tskSeguridad.Stop();
    Serial.println("La contraseña fue correcta");
  });
  maquinaEstados.SetOnLeaving(Configuracion, []() {
    tskConfiguracion.Stop();
    Serial.println("Saliendo de la configuracion");
  });
  maquinaEstados.SetOnLeaving(Monitoreo, []() {
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

void botonPresionado() {
  Serial.println("Boton Presionado");
  if (maquinaEstados.GetState() == Estado::Monitoreo) {
    Serial.println("Pasando a configuracion");
    maquinaEstados.setEntradaActual(Entrada::Configurar);
  }
  else if (maquinaEstados.GetState() == Estado::Configuracion) {
    Serial.println("Pasando a monitorear");
    lcd.clear();
    maquinaEstados.setEntradaActual(Entrada::Monitorear);
  }
  else if (maquinaEstados.GetState() == Estado::Alarma) {
    Serial.println("Pasando a configuracion desde alarma");
    maquinaEstados.setEntradaActual(Entrada::Configurar);
  }
}

void setup() {
  pinMode(PIN_BOTON, INPUT_PULLUP);

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

void loop() {
  maquinaEstados.Update();
  tskSeguridad.Update();
  tskConfiguracion.Update();
  tskMonitoreo.Update();
  tskAlarma.Update();

}
