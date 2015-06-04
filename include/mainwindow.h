#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>

#include <QMainWindow>
#include <QPixmap>
#include <QString>

#include "celda.h"

// Declaración adelantada de clases para no incluir aquí todas las cabeceras.
class Cortadora;
class QProgressBar;
class QGraphicsScene;
class QGraphicsRectItem;

namespace Ui {
class MainWindow;
}

// La clase MainWindow contiene y manipula internamente todos los objetos
// gráficos y se hace cargo del control del flujo de ejecución, llamando a las
// funciones necesarias cuando el usuario así lo desea.
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  // Constructor y destructor
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

  // Modificación del jardín
  void ImgMod(int fila, int columna, const TipoCelda& tipo);
  void resize(int filas, int columnas);
  void set_pos(int fila, int columna, const TipoCelda& tipo);

  // Funciones de guardado y de carga
  void save();
  void load();

  // Accesores
  int columnas() const { return columns; }
  int filas() const { return rows; }
  int get_ini_x() const { return ini_x; }
  int get_ini_y() const { return ini_y; }
  int get_fin_x() const { return fin_x; }
  int get_fin_y() const { return fin_y; }
  Celda* get_pos(int fila, int columna) const { return label_list[fila][columna]; }

public slots:
  void on_bAleatorio_clicked();

private slots:
  // Código ejecutado al pulsar botones
  void on_bCamino_clicked();
  void on_bPruebas_clicked();
  void on_bReset_clicked();
  void on_bSimular_clicked();
  void on_cbEdicion_clicked(bool checked);
  void on_Celda_clicked(int fila, int columna);
  void on_sbColumnas_valueChanged(int columnas);
  void on_sbFilas_valueChanged(int filas);

  // Acciones
  void on_actionAbrir_triggered();
  void on_actionAcerca_de_triggered();
  void on_actionGuardar_triggered();
  void on_actionGuardar_como_triggered();
  void on_actionNuevo_triggered();
  void on_actionSalir_triggered();

private:
  void lock_interface(bool b);

private:
  // Atributos del programa principal
  Ui::MainWindow* ui;
  QString filename;
  QProgressBar* progressBar;
  QGraphicsScene* scene;

  // Atributos del jardín
  int rows, columns;
  int ini_x, ini_y;
  int fin_x, fin_y;
  Cortadora* corta;

  // Matriz que representa el jardín, compuesta por celdas y matriz con los
  // rectángulos que forman el minimapa, para poder eliminarlos al
  // redimensionar el jardín.
  std::vector<std::vector<Celda*> > label_list;
  std::vector<std::vector<QGraphicsRectItem*> > rect_list;

  // Imágenes de representación de cada celda
  const QPixmap cesped_a;
  const QPixmap cesped_b;
  const QPixmap obstaculo;
  const QPixmap inicio;
  const QPixmap cortadora;
  const QPixmap punto_a;
  const QPixmap punto_b;
};

#endif // MAINWINDOW_H
