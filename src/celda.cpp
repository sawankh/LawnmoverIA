#include "celda.h"
#include <QMouseEvent>

Celda::Celda(int fila, int columna, const QString &text, QWidget* parent):
    QLabel(text, parent), row(fila), column(columna), tipo_(CESPED_A)
{
}

void Celda::setTipo(const TipoCelda& tipo){
  tipo_ = tipo;
}

TipoCelda Celda::tipo() const {
  return tipo_;
}

// Si la celda es pulsada por el botón izquierdo del ratón, se emite la señal
// clicked(), que es capturada por la ventana principal y que hace que se
// cambie el tipo de la celda.
void Celda::mousePressEvent(QMouseEvent* event){
  if(event->button() & Qt::LeftButton)
    emit clicked(row, column);
}
