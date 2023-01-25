#include <LiquidMenu.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <AsyncTaskLib.h>
#include <DHTStable.h>
#include <Config.h>
#include <EasyBuzzer.h>
#include <EEPROM.h>
#include "StateMachineLib.h"
#include "Configuracion.h"
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
    loop();
    Serial.println("Here");
  }
});

#pragma endregion

#pragma region menu
AsyncTask tskMenu(100, []() {

  auto setup = []() {

  };

  auto loop = []() {

  };

  setup();
  loop();
});

#pragma endregion
AsyncTask tskMonitoreo(100, []() {
  auto setup = []() {

  };

  auto loop = []() {

  };

  setup();
  loop();
});
AsyncTask tskAlarma(100, []() {
  auto setup = []() {

  };

  auto loop = []() {

  };

  setup();
  loop();
});
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

  });
  maquinaEstados.SetOnEntering(Monitoreo, []() {});
  maquinaEstados.SetOnEntering(Alarma, []() {});

  //Al Salir
  maquinaEstados.SetOnLeaving(Inicio, []() {
    Serial.println("Leaving A");
  });
  maquinaEstados.SetOnLeaving(Configuracion, []() {
    Serial.println("Leaving A");
  });
  maquinaEstados.SetOnLeaving(Monitoreo, []() {
    Serial.println("Leaving A");
  });
  maquinaEstados.SetOnLeaving(Alarma, []() {
    Serial.println("Leaving A");
  });
}

#pragma endregion

void botonPresionado() {
  Serial.println("Boton Presionado");
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

}
