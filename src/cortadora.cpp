#include "cortadora.h"

#include <cmath>
#include <stack>

#include <QCoreApplication>
#include <QMessageBox>
#include <QTime>

#include "mainwindow.h"

// Hace una espera ocupada procesando eventos durante el tiempo especificado.
void qSleep(int ms){
  QTime dieTime= QTime::currentTime().addMSecs(ms);
  while (QTime::currentTime() < dieTime)
    QCoreApplication::processEvents();
}

// El constructor inicializa la posición inicial de la cortadora y asigna una
// velocidad de movimiento por defecto.
Cortadora::Cortadora(MainWindow* padre, int fila, int columna): QObject(padre),
  father(padre), row(fila), column(columna), delay(500)
{
}

// Este es el método que decide el movimiento inicial que realiza la cortadora
// cuando va a cortar todo el césped.
void Cortadora::cortar_cesped(int* iteraciones){

  // Si los puntos A y B están colocados, se sustituyen por césped para que no
  // haya problemas al ejecutar el algoritmo
  if(father->get_ini_x() != -1)
    father->set_pos(father->get_ini_y(), father->get_ini_x(), CESPED_A);
  if(father->get_fin_x() != -1)
    father->set_pos(father->get_fin_y(), father->get_fin_x(), CESPED_A);

  // Intenta comenzar en todas las direcciones posibles si no se ha pasado
  // todavía por alguna de ellas para dar el primer paso antes de la función
  // recursiva

  if(!hay_obstaculo(ARRIBA) && father->get_pos(row-1, column)->tipo() != CESPED_B){
    // Cada vez que se va hacia una dirección, tanto aquí como en la función
    // recursiva, se debe hacer el camino y luego hacer el movimiento contrario
    // para volver a la posición inicial sin dar saltos
    cortar(ARRIBA, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(ABAJO, iteraciones);
  }
  if(!hay_obstaculo(ABAJO) && father->get_pos(row+1, column)->tipo() != CESPED_B){
    cortar(ABAJO, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(ARRIBA, iteraciones);
  }
  if(!hay_obstaculo(IZQUIERDA) && father->get_pos(row, column-1)->tipo() != CESPED_B){
    cortar(IZQUIERDA, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(DERECHA, iteraciones);
  }
  if(!hay_obstaculo(DERECHA) && father->get_pos(row, column+1)->tipo() != CESPED_B){
    cortar(DERECHA, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(IZQUIERDA, iteraciones);
  }
  father->set_pos(row, column, CORTADORA);
}

// A partir de la posición actual de la cortadora, intenta alcanzar el punto
// final especificado como parámetro utilizando un algoritmo de búsqueda
// heurístico.
void Cortadora::reach(int fila, int columna, int* iteraciones){

  // Cada una de las distancias es la que hay desde cada celda adyacente al
  // cortacésped hasta el punto final
  int dist[4];
  int minimo, indice;
  father->set_pos(row, column, CORTADORA);

  // Aquí se van a almacenar las direcciones que va tomando la cortadora para
  // que las pueda recordar y poder volver atrás para buscar otro camino si se
  // encierra
  std::stack<int> camino;

  while(row != fila || column != columna){
    qSleep(delay);

    // Las distancias y el mínimo se inicializan a -1 porque es un valor no
    // válido y fácilmente reconocible
    minimo = dist[0] = dist[1] = dist[2] = dist[3] = -1;
    indice = 0;

    // Las distancias de cada posición están en los siguientes índices:
    // ARRIBA: 0
    // ABAJO: 1
    // IZQUIERDA: 2
    // DERECHA: 3
    if(!hay_obstaculo(ARRIBA) &&
       father->get_pos(row-1, column)->tipo() != CESPED_B)
      dist[0] = abs(fila - (row-1)) + abs(columna - column);
    if(!hay_obstaculo(ABAJO) &&
       father->get_pos(row+1, column)->tipo() != CESPED_B)
      dist[1] = abs(fila - (row+1)) + abs(columna - column);
    if(!hay_obstaculo(IZQUIERDA) &&
       father->get_pos(row, column-1)->tipo() != CESPED_B)
      dist[2] = abs(fila - row) + abs(columna - (column-1));
    if(!hay_obstaculo(DERECHA) &&
       father->get_pos(row, column+1)->tipo() != CESPED_B)
      dist[3] = abs(fila - row) + abs(columna - (column+1));

    // Calculamos el mínimo de todas las distancias descartando valores nulos,
    // porque éstos representan obstáculos o posiciones fuera de rango
    for(int i = 0; i < 4; ++i){
      if(minimo < 0 || (dist[i] >= 0 && dist[i] < minimo)){
        minimo = dist[i];
        indice = i;
      }
    }

    father->set_pos(row, column, CESPED_B);

    // Si la distancia mínima calculada es un valor nulo, es porque se ha
    // encerrado
    if(dist[indice] == -1){
      // Si no puede volver atrás es porque no ha encontrado un camino hasta el
      // destino
      if(camino.empty()){
        QMessageBox::critical(father, "Error",
                              "No se ha podido llegar al punto de destino.");
        return;
      }
      // Si puede volver atrás, entonces se deshace el último movimiento
      // realizado eliminándolo de la pila y haciendo el movimiento contrario
      else {
        indice = camino.top();
        camino.pop();
        switch(indice){
          case 0:
            indice = 1;
            break;
          case 1:
            indice = 0;
            break;
          case 2:
            indice = 3;
            break;
          case 3:
            indice = 2;
            break;
        }
      }
    }
    else
      // Si se ha podido mover a una celda no repetida y sin obstáculos,
      // guardamos el movimiento para luego poder deshacerlo si el camino
      // elegido no tiene salida
      camino.push(indice);

    switch(indice){
      case 0:
        mover(ARRIBA, iteraciones);
        break;
      case 1:
        mover(ABAJO, iteraciones);
        break;
      case 2:
        mover(IZQUIERDA, iteraciones);
        break;
      case 3:
        mover(DERECHA, iteraciones);
        break;
    }
    father->set_pos(row, column, CORTADORA);
  }
}

// Coloca la cortadora en otra posición.
void Cortadora::ir_a(int fila, int columna){
  row = fila;
  column = columna;
}

bool Cortadora::hay_obstaculo(Movimientos mov) const {

  // Para la cortadora hay un obstáculo si existe un obstáculo en la dirección
  // indicada o esa dirección está fuera de los límites del jardín
  switch(mov){
  case ARRIBA:
    return ((row == 0) ||
            (father->get_pos(row-1, column)->tipo() == OBSTACULO)||
            (father->get_pos(row-1, column)->tipo() == INICIO));
  case ABAJO:
    return ((row == father->filas()-1) ||
            (father->get_pos(row+1, column)->tipo() == OBSTACULO) ||
            (father->get_pos(row+1, column)->tipo() == INICIO));
  case IZQUIERDA:
    return ((column == 0) ||
            (father->get_pos(row, column-1)->tipo() == OBSTACULO) ||
            (father->get_pos(row, column-1)->tipo() == INICIO));
  case DERECHA:
    return ((column == father->columnas()-1) ||
            (father->get_pos(row, column+1)->tipo() == OBSTACULO) ||
            (father->get_pos(row, column+1)->tipo() == INICIO));
  }
  return false;
}

void Cortadora::mover(Movimientos mov, int* iteraciones){
  switch(mov){
  case ARRIBA:
    --row;
    break;
  case ABAJO:
    ++row;
    break;
  case IZQUIERDA:
    --column;
    break;
  case DERECHA:
    ++column;
    break;
  }
  if(iteraciones) ++(*iteraciones);
}

void Cortadora::on_delay_changed(int value){
  delay = value;
}

// Función recursiva que realiza un recorrido en profundidad del jardín.
void Cortadora::cortar(Movimientos mov, int* iteraciones){

  // En este punto, la cortadora se encuentra en la posición que tenía antes de
  // moverse. Debemos actualizar en la pantalla la imagen de la posición
  // antigua dependiendo de si era el punto de inicio o no.
  if(row == 0 && column == 0)
    father->set_pos(row, column, INICIO);
  else
    father->set_pos(row, column, CESPED_B);

  // Aquí se realiza el cambio de posición de la cortadora y se actualiza la
  // imagen de la posición
  mover(mov, iteraciones);
  father->set_pos(row, column, CORTADORA);
  qSleep(delay);

  // Se prueba en todas las direcciones a moverse en el caso de que no haya un
  // obstáculo ni se salga de los límites del mapa ni haya pasado y cortado el
  // césped previamente

  if(!hay_obstaculo(ARRIBA) && father->get_pos(row-1, column)->tipo() != CESPED_B){
    // Aquí se llama recursivamente a la misma función para que corte todo el
    // camino posible en esa dirección. Cuando acaba, se mueve de nuevo hacia
    // el punto original para que la vuelta de la recursión lleve a la
    // cortadora al punto de inicio.
    cortar(ARRIBA, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(ABAJO, iteraciones);
    father->set_pos(row, column, CORTADORA);
    qSleep(delay);
  }
  if(!hay_obstaculo(ABAJO) && father->get_pos(row+1, column)->tipo() != CESPED_B){
    cortar(ABAJO, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(ARRIBA, iteraciones);
    father->set_pos(row, column, CORTADORA);
    qSleep(delay);
  }
  if(!hay_obstaculo(IZQUIERDA) && father->get_pos(row, column-1)->tipo() != CESPED_B){
    cortar(IZQUIERDA, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(DERECHA, iteraciones);
    father->set_pos(row, column, CORTADORA);
    qSleep(delay);
  }
  if(!hay_obstaculo(DERECHA) && father->get_pos(row, column+1)->tipo() != CESPED_B){
    cortar(DERECHA, iteraciones);
    father->set_pos(row, column, CESPED_B);
    mover(IZQUIERDA, iteraciones);
    father->set_pos(row, column, CORTADORA);
    qSleep(delay);
  }
}
