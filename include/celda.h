#ifndef CELDA_H
#define CELDA_H

#include <QLabel>

// Los tipos de celda son las distintas cosas que pueden haber en cada una de
// las secciones del jardín.
enum TipoCelda {CESPED_A, CESPED_B, OBSTACULO, INICIO, CORTADORA, PUNTO_A, PUNTO_B};

// Las celdas son cada una de las imágenes que representan una sección unitaria
// del jardín.
class Celda: public QLabel {
  Q_OBJECT

public:
  Celda(int fila, int columna, const QString& text = "", QWidget* parent = 0);
  void setTipo(const TipoCelda& tipo);
  TipoCelda tipo() const;

signals:
  void clicked(int, int);

protected:
  void mousePressEvent(QMouseEvent* event);

private:
  int row, column;
  TipoCelda tipo_;
};

#endif // CELDA_H
