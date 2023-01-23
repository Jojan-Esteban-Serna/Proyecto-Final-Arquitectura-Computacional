#include <LiquidMenu.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <AsyncTaskLib.h>
#include <DHTStable.h>
#include <Config.h>
#include <EasyBuzzer.h>
#include <EEPROM.h>
#include <StateMachineLib.h>
#include "Configuracion.h"

#pragma region configuracion_teclado
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 23, 25, 27, 29 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 31, 33, 35, 37 };  //connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
#pragma endregion

#pragma region configuracion_lcd
#if defined(LiquidCrystal_I2C_h)
LiquidCrystal_I2C lcd(0x27, 16, 2);
#elif defined(LiquidCrystal_h)
const int rs = 7, en = 8, d4 = 22, d5 = 24, d6 = 26, d7 = 28;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#endif
#pragma endregion



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
  maquinaEstados.SetOnEntering(Inicio, []() {});
  maquinaEstados.SetOnEntering(Configuracion, []() {});
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
void setup() {
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
}
