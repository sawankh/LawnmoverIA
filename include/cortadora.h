#ifndef CORTADORA_H
#define CORTADORA_H

#include <QObject>

class MainWindow;

enum Movimientos {ARRIBA, ABAJO, IZQUIERDA, DERECHA};

class Cortadora: public QObject {
    Q_OBJECT

public:
  Cortadora(MainWindow* padre, int fila, int columna);
  ~Cortadora(){}

  // Corta todo el césped que puede alcanzar la cortadora. Utiliza una
  // estrategia de búsqueda en profundidad.
  void cortar_cesped(int* iteraciones = NULL);

  // Intenta llegar al punto especificado del jardín.
  // Emplea una estrategia voraz: Algoritmo de escalada.
  void reach(int fila, int columna, int* iteraciones = NULL);

  // Cambia la posición actual de la cortadora sin más efectos secundarios.
  void ir_a(int fila, int columna);

  // Sensores
  bool hay_obstaculo(Movimientos mov) const;

  // Actuadores
  void mover(Movimientos mov, int* iteraciones = NULL);

public slots:
  // Cambia la velocidad de la simulación.
  void on_delay_changed(int value);

protected:
  // Función recursiva que permite realizar la búsqueda en profundidad para
  // cortar todo el césped.
  void cortar(Movimientos mov, int* iteraciones = NULL);

private:
  MainWindow* father;
  int row, column;
  int delay;
};

#endif // CORTADORA_H
