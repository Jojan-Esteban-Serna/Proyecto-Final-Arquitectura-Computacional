#include <LiquidMenu.h>
#include <LiquidCrystal.h>
#include <AsyncTaskLib.h>
#include <DHTStable.h>
#include <Config.h>
#include <EasyBuzzer.h>
#include <EEPROM.h>
#include <StateMachineLib.h>
#include "Configuracion.h"
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
  TemperaturaCaliente
};



class MaquinaDeEstados : public StateMachine {
private:
  Entrada entradaActual;
public:


  MaquinaDeEstados()
    : StateMachine(4, 5) {
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
  Serial.begin(9600);

  Serial.println("Starting State Machine...");
  configurarMaquinaEstado();
  Serial.println("Start Machine Started");

  maquinaEstados.SetState(Inicio, false, true);
}

void loop() {
  maquinaEstados.Update();
}
