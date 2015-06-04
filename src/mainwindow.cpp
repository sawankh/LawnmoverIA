#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>
#include <ctime>

#include <QFile>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QProgressBar>
#include <QString>
#include <QTime>

#include "celda.h"
#include "cortadora.h"

// Tamaño por defecto del jardín.
static const int ROWS = 50;
static const int COLUMNS = 50;

// Restricciones de dimensionamiento.
static const int MIN_ROWS = 5;
static const int MIN_COLUMNS = 5;
static const int MAX_ROWS = 150;
static const int MAX_COLUMNS = 150;
static const int MINIMAP_CELL_WIDTH = 5;
static const int MINIMAP_CELL_HEIGHT = 5;

// Cantidad relativa de obstáculos con respecto a césped al generar el jardín
// aleatoriamente.
static const int PORCENTAJE_OBSTACULOS = 20;

/*
 * CONSTRUCTOR Y DESTRUCTOR
 */

// El constructor inicializa el jardín sin obstáculos ni puntos de inicio o
// final, evitando en la medida de lo posible que se retrase la carga del
// programa.
// También se cargan las imágenes de cada tipo de celda, que luego se mostrarán
// cundo sea necesario sin ocupar memoria adicional.
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent),
    ui(new Ui::MainWindow), filename(""), progressBar(NULL), scene(NULL),
    rows(0), columns(0), ini_x(-1), ini_y(-1), fin_x(-1), fin_y(-1),
    corta(NULL), cesped_a(":/resources/cesped_a.png"),
    cesped_b(":/resources/cesped_b.png"), obstaculo(":/resources/obstaculo.png"),
    inicio(":/resources/inicio.png"), cortadora(":/resources/cortadora.jpg"),
    punto_a(":/resources/A.png"), punto_b(":/resources/B.png") {

  srand(time(NULL));
  ui->setupUi(this);
  setWindowTitle("IA - Búsqueda: <Sin Nombre>");

  // Creación de algunos elementos que no se crean automáticamente por Qt
  progressBar = new QProgressBar(this);
  corta = new Cortadora(this, 0, 0);
  scene = new QGraphicsScene(this);

  // Configuración inicial de la interfaz
  ui->cbEdicion->setChecked(true);
  ui->fEditar->setEnabled(true);
  ui->fSimular->setEnabled(false);
  ui->actionSalir->setShortcut(QKeySequence(Qt::ALT + Qt::Key_F4));
  ui->statusBar->addWidget(progressBar);
  ui->gridLayout->setSpacing(0);
  ui->graphicsView->setScene(scene);

  ui->sbFilas->setMinimum(MIN_ROWS);
  ui->sbFilas->setMaximum(MAX_ROWS);
  ui->sbColumnas->setMinimum(MIN_COLUMNS);
  ui->sbColumnas->setMaximum(MAX_COLUMNS);

  showMaximized();

  // Conectamos el evento de mover el control deslizante con la cortadora para
  // que cambie su velocidad de ejecución.
  connect(ui->timeSlider, SIGNAL(sliderMoved(int)),
          corta, SLOT(on_delay_changed(int)));

  // Creamos el jardín con el tamaño inicial por defecto
  resize(ROWS, COLUMNS);
}

MainWindow::~MainWindow(){
  for(int i = 0; i < rows; ++i){
    for(int j = 0; j < columns; ++j){
      delete label_list[i][j];
      delete rect_list[i][j];
    }
  }

  delete progressBar;
  delete corta;
  delete scene;
  delete ui;
}

/*
 * MODIFICACIÓN DEL JARDÍN
 */

// Este método modifica la imagen que se muestra en una determinada posición
// del jardín en la pantalla. Sólo modifica la imagen en la pantalla, no afecta
// a la ejecución de los algoritmos.
// Además de eso, también actualiza la representación en el minimapa.
void MainWindow::ImgMod(int fila, int columna, const TipoCelda& tipo){

  // Este rectángulo es el que se rellenará de color para representar el
  // contenido de una celda en el minimapa
  QRect rect(columna*(MINIMAP_CELL_WIDTH),
             fila*(MINIMAP_CELL_HEIGHT),
             MINIMAP_CELL_WIDTH,
             MINIMAP_CELL_HEIGHT);
  QColor color;

  switch(tipo){
  case CESPED_A:
    label_list[fila][columna]->setPixmap(cesped_a);
    color.setRgb(0, 128, 0);
    break;
  case CESPED_B:
    label_list[fila][columna]->setPixmap(cesped_b);
    color.setRgb(0, 187, 0);
    break;
  case OBSTACULO:
    label_list[fila][columna]->setPixmap(obstaculo);
    color.setRgb(128, 64, 0);
    break;
  case INICIO:
    label_list[fila][columna]->setPixmap(inicio);
    color.setRgb(255, 0, 0);
    break;
  case CORTADORA:
    label_list[fila][columna]->setPixmap(cortadora);
    color.setRgb(255, 0, 0);
    break;
  case PUNTO_A:
    label_list[fila][columna]->setPixmap(punto_a);
    color.setRgb(0, 128, 255);
    break;
  case PUNTO_B:
    label_list[fila][columna]->setPixmap(punto_b);
    color.setRgb(255, 32, 64);
    break;
  }

  if(rect_list[fila][columna])
    delete rect_list[fila][columna];
  rect_list[fila][columna] = scene->addRect(rect, QPen(Qt::NoPen), QBrush(color));
}

// Este método redimensiona el jardín.
//
// Como está implementado con vectores de tamaño dinámico, lo que hace esta
// función, en lugar de eliminar todo el jardín y crear uno nuevo con otro
// tamaño, es añadir o borrar tantas filas y columnas como se necesite, haciendo
// que cambios de tamaño pequeños sean más rápidos.
//
// Todas las celdas nuevas que se crean se conectan a esta clase para que
// cuando un usuario pulsa sobre alguna de las celdas, la ventana principal lo
// sepa y pueda cambiar su contenido.
void MainWindow::resize(int filas, int columnas){

  // Redimensionamos el minimapa para que aproveche todo el espacio posible
  ui->graphicsView->setMaximumHeight(MINIMAP_CELL_HEIGHT*(filas+1));
  ui->graphicsView->setMaximumWidth(MINIMAP_CELL_WIDTH*(columnas+1));
  scene->setSceneRect(0, 0, MINIMAP_CELL_WIDTH*columnas, MINIMAP_CELL_HEIGHT*filas);

  // Calculamos el número total de modificaciones que se van a hacer para poder
  // calcular un porcentaje mientras se realiza y mostrar así la barra de
  // progreso
  int totalmods = (filas>rows? 2*(filas-rows)*columns :
                  (rows-filas)*label_list[label_list.size()-1].size()) +
                  (label_list.size() *
                  (std::max(columnas, columns) - std::min(columnas, columns)));
  int donemods = 0;

  progressBar->setHidden(false);

  // Añadir filas al jardín
  if(filas > rows){
    for(int i = 0; i < filas-rows; ++i){
      std::vector<Celda*> aux(columns);
      for(int j = 0; j < columns; ++j){
        aux[j] = new Celda(label_list.size(), j, "", this);
        ++donemods;
      }
      label_list.push_back(aux);
      rect_list.push_back(std::vector<QGraphicsRectItem*>(columns, static_cast<QGraphicsRectItem*>(NULL)));
      for(int j = 0; j < columns; ++j){
        connect(label_list[label_list.size()-1][j], SIGNAL(clicked(int, int)),
                this, SLOT(on_Celda_clicked(int, int)));
        ui->gridLayout->addWidget(label_list[label_list.size()-1][j], label_list.size()-1, j);
        ImgMod(label_list.size()-1, j, label_list[label_list.size()-1][j]->tipo());
        ++donemods;
      }
      progressBar->setValue((donemods*100)/(totalmods>0? totalmods : 1));
    }
  }

  // Eliminar filas del jardín
  else if(filas < rows){
    for(int i = 0; i < rows-filas; ++i){
      for(unsigned j = 0; j < label_list[label_list.size()-1].size(); ++j){
        delete label_list[label_list.size()-1][j];
        if(rect_list[rect_list.size()-1][j])
          delete rect_list[rect_list.size()-1][j];
        ++donemods;
      }
      label_list.pop_back();
      rect_list.pop_back();
      progressBar->setValue((donemods*100)/(totalmods>0? totalmods : 1));
    }

    // Si el punto de inicio o de fin de la simulación estaba en la fila que se
    // borró, se invalida
    if(ini_y >= filas) ini_x = -1;
    if(fin_y >= filas) fin_x = -1;
  }

  // Añadir columnas al jardín
  if(columnas > columns){
    for(unsigned i = 0; i < label_list.size(); ++i){
      for(int j = 0; j < columnas-columns; ++j){
        label_list[i].push_back(new Celda(i, label_list[i].size(), "", this));
        rect_list[i].push_back(NULL);

        connect(label_list[i][label_list[i].size()-1],SIGNAL(clicked(int, int)), this, SLOT(on_Celda_clicked(int, int)));
        ui->gridLayout->addWidget(label_list[i][label_list[i].size()-1], i, label_list[i].size()-1);
        ImgMod(i, label_list[i].size()-1, label_list[i][label_list[i].size()-1]->tipo());
        donemods += 2;
      }
      progressBar->setValue((donemods*100)/(totalmods>0? totalmods : 1));
    }
  }

  // Eliminar columnas del jardín
  else if(columnas < columns){
    for(unsigned i = 0; i < label_list.size(); ++i){
      for(int j = 0; j < columns-columnas; ++j){
        delete label_list[i][label_list[i].size()-1];
        if(rect_list[i][rect_list[i].size()-1])
          delete rect_list[i][rect_list[i].size()-1];
        label_list[i].pop_back();
        rect_list[i].pop_back();
        ++donemods;
      }
      progressBar->setValue((donemods*100)/(totalmods>0? totalmods : 1));
    }

    // Si el punto de inicio o de fin de la simulación estaba en la fila que se
    // borró, se invalida
    if(ini_x >= columnas) ini_x = -1;
    if(fin_x >= columnas) fin_x = -1;
  }

  // Actualizamos las filas y columnas en la clase y los cuadros de
  // desplazamiento de la ventana
  rows = filas;
  columns = columnas;
  ui->sbFilas->setValue(filas);
  ui->sbColumnas->setValue(columnas);

  if(ini_x >= 0)
    ImgMod(ini_y, ini_x, PUNTO_A);
  if(fin_x >= 0)
    ImgMod(fin_y, fin_x, PUNTO_B);

  progressBar->setHidden(true);
  set_pos(0, 0, INICIO);
}

// La posición indicada pasa a ser del tipo indicado, teniendo efecto tanto en
// la pantalla como la ejecución de los algoritmos de la cortadora.
void MainWindow::set_pos(int fila, int columna, const TipoCelda& tipo){
  label_list[fila][columna]->setTipo(tipo);
  ImgMod(fila, columna, tipo);
}

/*
 * FUNCIONES DE GUARDADO Y DE CARGA
 */

// Guarda el contenido del jardín en el fichero indicado por "filename".
void MainWindow::save(){
  on_bReset_clicked();

  QFile out(filename);

  if(out.open(QIODevice::WriteOnly)){
    TipoCelda aux;
    int donewrites = 0;

    progressBar->setEnabled(true);

    out.write((char*)&rows, sizeof(int));
    out.write((char*)&columns, sizeof(int));
    for(int i = 0; i < rows; ++i){
      for(int j = 0; j < columns; ++j){
        ++donewrites;
        aux = label_list[i][j]->tipo();
        out.write((char*)&aux, sizeof(TipoCelda));
      }
      progressBar->setValue((donewrites*100)/(rows*columns));
    }
    out.write((char*)&ini_x, sizeof(int));
    out.write((char*)&ini_y, sizeof(int));
    out.write((char*)&fin_x, sizeof(int));
    out.write((char*)&fin_y, sizeof(int));
    out.flush();
    out.close();

    progressBar->setEnabled(false);
    setWindowTitle(QString("IA - Búsqueda: <") + filename + QString(">"));
  }
  else
    QMessageBox::critical(this, "Error al guardar",
                          "No se ha podido abrir el fichero para guardar. Compruebe sus permisos.");
}

// Carga el contenido del jardín desde el fichero indicado por "filename".
void MainWindow::load(){
  QFile in(filename);

  if(in.open(QIODevice::ReadOnly)){
    int filas, columnas, donereads = 0;
    TipoCelda aux;

    progressBar->setEnabled(true);

    in.read((char*)&filas, sizeof(int));
    in.read((char*)&columnas, sizeof(int));

    if(filas < MIN_ROWS || filas > MAX_ROWS || columnas < MIN_COLUMNS ||
       columnas > MAX_COLUMNS){
      QMessageBox::critical(this, "Error de lectura",
                            "El archivo especificado parece estar dañado o ser de otra aplicación. Imposible abrir.");
      in.close();
      return;
    }

    resize(filas, columnas);

    for(int i = 0; i < filas; ++i){
      for(int j = 0; j < columnas; ++j){
        ++donereads;
        if(in.read((char*)&aux, sizeof(TipoCelda)) == -1){
          QMessageBox::critical(this, "Error de lectura",
                                "El archivo especificado parece estar dañado o ser de otra aplicación. Imposible abrir.");
          in.close();
          progressBar->setEnabled(false);
          return;
        }
        set_pos(i, j, aux);
      }
      progressBar->setValue((donereads*100)/(filas*columnas));
    }
    in.read((char*)&ini_x, sizeof(int));
    in.read((char*)&ini_y, sizeof(int));
    in.read((char*)&fin_x, sizeof(int));
    in.read((char*)&fin_y, sizeof(int));
    in.close();

    progressBar->setEnabled(false);

    if(ini_x >= 0)
      set_pos(ini_y, ini_x, PUNTO_A);
    if(fin_x >= 0)
      set_pos(fin_y, fin_x, PUNTO_B);

    setWindowTitle(QString("IA - Búsqueda: <") + filename + QString(">"));
  }
  else
    QMessageBox::critical(NULL, "Error al cargar",
                          "No se ha podido abrir el fichero para cargar. Compruebe sus permisos.");
}

// Recorre todo el jardín colocando aleatoriamente obstáculos y luego sitúa
// también aleatoriamente los puntos inicial y final.
void MainWindow::on_bAleatorio_clicked(){
  progressBar->setHidden(false);

  int iteraciones = 0;
  for(int i = 0; i < rows; ++i){
    for(int j = 0; j < columns; ++j){
      if(!(i == 0 && j == 0)){
        if(rand()%100 < PORCENTAJE_OBSTACULOS)
          set_pos(i, j, OBSTACULO);
        else
          set_pos(i, j, CESPED_A);
      }
      ++iteraciones;
    }
    progressBar->setValue(iteraciones*100/(rows*columns));
  }

  do {
    ini_x = rand() % columns;
    ini_y = rand() % rows;
  } while(ini_x == 0 && ini_y == 0);
  do {
  fin_x = rand() % columns;
  fin_y = rand() % rows;
  } while(fin_x == 0 && fin_y == 0);

  set_pos(ini_y, ini_x, PUNTO_A);
  set_pos(fin_y, fin_x, PUNTO_B);

  progressBar->setHidden(true);
}

/*
 * CÓDIGO EJECUTADO AL PULSAR BOTONES
 */

// Prepara la interfaz y la cortadora y, si es posible, ejecuta la simulación
// de ir del punto de inicio al punto final.
void MainWindow::on_bCamino_clicked(){
  if(ini_x < 0 || fin_x < 0){
    QMessageBox::critical(this, "Error",
                          "Falta el punto de inicio o de fin del recorrido.",
                          QMessageBox::Ok);
    return;
  }

  on_bReset_clicked();
  corta->ir_a(ini_y, ini_x);

  set_pos(0, 0, INICIO);

  lock_interface(true);
  corta->on_delay_changed(ui->timeSlider->value());
  corta->reach(fin_y, fin_x);
  lock_interface(false);
}

// Ejecuta el algoritmo de corte de todo el céspedy del camino entre dos puntos
// y calcula para cada uno el número de pasos que dio la cortadora y el tiempo
// que tardó. Para el corte de todo el césped mide el porcentaje de césped
// cortado.
void MainWindow::on_bPruebas_clicked(){
  ui->timeSlider->setEnabled(false);
  lock_interface(true);

  // Corte de todo el césped
  QTime time;
  int sim_iter = 0, cam_iter = 0;
  int sim_time, cam_time = 0;
  int cesped_total = 0, cesped_cortado = 0;
  on_bReset_clicked();

  corta->ir_a(0, 0);
  corta->on_delay_changed(0);

  time.start();
  corta->cortar_cesped(&sim_iter);
  sim_time = time.elapsed();

  for(int i = 0; i < rows; ++i){
    for(int j = 0; j < columns; ++j){
      switch(label_list[i][j]->tipo()){
      case CESPED_B:
        ++cesped_cortado;
      case CESPED_A:
        ++cesped_total;
          set_pos(i, j, CESPED_A);
        break;
      default:
        break;
      }
    }
  }

  // Corte del camino entre dos puntos
  if(ini_x < 0 || fin_x < 0)
    QMessageBox::critical(this, "Error",
                          "Falta el punto de inicio o de fin del recorrido.",
                          QMessageBox::Ok);
  else {
    corta->ir_a(ini_y, ini_x);
    set_pos(0, 0, INICIO);
    corta->on_delay_changed(0);

    time.start();
    corta->reach(fin_y, fin_x, &cam_iter);
    cam_time = time.elapsed();
  }

  switch(QMessageBox::information(this, "Resultados",
                                  "Porcentaje de césped cortado: " + QString::number((cesped_cortado*100)/static_cast<double>(cesped_total)) + "%\n\n"
                                  "Número de iteraciones realizadas:\n"
                                  "-Cortar todo el césped: " + QString::number(sim_iter) + "\n"
                                  "-Cortar camino: " + QString::number(cam_iter) + "\n\n"
                                  "Tiempo transcurrido:\n"
                                  "-Cortar todo el césped: " + QString::number(sim_time) + "ms\n"
                                  "-Cortar camino: " + QString::number(cam_time) + "ms\n\n"
                                  "¿Deseas exportar los resultados a un fichero de texto?",
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)){
  case QMessageBox::Yes:
  {
    QString dir = QFileDialog::getSaveFileName(this, "Archivo de destino", "", "Archivos de texto (*.txt)");
    if(dir.length() > 0){
      QFile out(dir);
      if(out.open(QIODevice::WriteOnly | QIODevice::Text)){
        out.write("---===RESULTADOS DE LAS PRUEBAS===---\n\n");
        out.write(QString("Porcentaje de césped cortado: " +
                          QString::number((cesped_cortado*100)/static_cast<double>(cesped_total)) + "%\n").toStdString().c_str());
        out.write("Número de iteraciones realizadas:\n");
        out.write(QString("-Cortar todo el césped: " + QString::number(sim_iter) + "\n").toStdString().c_str());
        out.write(QString("-Corte camino entre 2 puntos: " + QString::number(cam_iter) + "\n").toStdString().c_str());
        out.write("Tiempo transcurrido:\n");
        out.write(QString("-Cortar todo el césped: " + QString::number(sim_time) + "ms\n").toStdString().c_str());
        out.write(QString("-Corte camino entre 2 puntos: " + QString::number(cam_time) + "ms\n").toStdString().c_str());
        out.flush();
        out.close();
      }
      else
        QMessageBox::critical(NULL, "Error al cargar",
                              "No se ha podido abrir el fichero para guardar. Compruebe sus permisos.");
    }
    break;
  }
  case QMessageBox::No:
  default:
    break;
  }

  lock_interface(false);
  ui->timeSlider->setEnabled(true);
  on_bReset_clicked();
}

// Convierte el césped cortado en césped alto y sitúa los puntos de inicio y
// fin para preparar al jardín para otra simulación.
void MainWindow::on_bReset_clicked(){
  int iteraciones = 0;

  progressBar->setHidden(false);

  for(int i = 0; i < rows; ++i){
    for(int j = 0; j < columns; ++j){
      if(label_list[i][j]->tipo() == CESPED_B)
        set_pos(i, j, CESPED_A);
      ++iteraciones;
    }
    progressBar->setValue(iteraciones*100/(rows*columns));
  }

  if(ini_x != -1)
    set_pos(ini_y, ini_x, PUNTO_A);
  if(fin_x != -1)
    set_pos(fin_y, fin_x, PUNTO_B);

  progressBar->setHidden(true);
}

// Ejecuta el algoritmo de cortar todo el jardín
void MainWindow::on_bSimular_clicked(){
  on_bReset_clicked();

  corta->ir_a(0, 0);

  lock_interface(true);
  corta->on_delay_changed(ui->timeSlider->value());
  corta->cortar_cesped();
  lock_interface(false);
}

// Alterna entre el modo edición y el modo simulación
void MainWindow::on_cbEdicion_clicked(bool checked){
  if(!checked)
    set_pos(0, 0, CORTADORA);
  else {
    set_pos(0, 0, INICIO);
    on_bReset_clicked();
  }
}

// Decide qué tipo de celda debe ser la celda especificada tras ser pulsada por
// el usuario. El orden es el siguiente:
// CESPED_A --> OBSTACULO --> PUNTO_A --> PUNTO_B --> CESPED_A...
// Sólo permite un punto A y un punto B en todo el jardín.
void MainWindow::on_Celda_clicked(int fila, int columna){
  if(ui->cbEdicion->isChecked()){
    TipoCelda tipo = label_list[fila][columna]->tipo();
    if(tipo != INICIO){
      if(tipo == CESPED_A)
        set_pos(fila, columna, OBSTACULO);
      else if(tipo == OBSTACULO){
        if(ini_x == -1){
          set_pos(fila, columna, PUNTO_A);
          ini_x = columna;
          ini_y = fila;
        }
        else {
          if(fin_x == -1){
            set_pos(fila, columna, PUNTO_B);
            fin_x = columna;
            fin_y = fila;
          }
          else
            set_pos(fila, columna, CESPED_A);
        }
      }
      else if(tipo == PUNTO_A){
        ini_x = -1;
        if(fin_x == -1){
          set_pos(fila, columna, PUNTO_B);
          fin_x = columna;
          fin_y = fila;
        }
        else
          set_pos(fila, columna, CESPED_A);
      }
      else {
        fin_x = -1;
        set_pos(fila, columna, CESPED_A);
      }
    }
  }
}

// Al cambiar el número de columnas, se realiza un redimensionamiento del
// jardín.
void MainWindow::on_sbColumnas_valueChanged(int columnas){
  resize(rows, columnas);
}

// Al cambiar el número de filas, se realiza un redimensionamiento del
// jardín.
void MainWindow::on_sbFilas_valueChanged(int filas){
  resize(filas, columns);
}

/*
 * ACCIONES
 */

// Al pulsar el menú "Abrir..." se pide al usuario un nombre de fichero antes
// de llamar a la función load().
void MainWindow::on_actionAbrir_triggered()
{
  filename = QFileDialog::getOpenFileName(this, "Abrir fichero...", "",
                                          "Archivos de jardín (*.garden)");
  if(filename.length() > 0){
    load();
    on_cbEdicion_clicked(true);
    ui->cbEdicion->setChecked(true);
  }
}

// Menú con los nombres de los autores.
void MainWindow::on_actionAcerca_de_triggered(){
  QMessageBox::about(this, "Acerca de...",
                     "Programa de búsqueda para la asignatura de Inteligencia "
                     "Artificial en la ULL. Curso 2013 - 2014.\n\n"
                     "Autores:\n"
                     "Sergio M. Afonso Fumero\n"
                     "Jorge Gómez Weyler\n"
                     "Sawan J. Kapai Harpalani");
}

// Si se pulsa guardar y se ha guardado previamente o se ha abierto algún
// fichero, se guarda directamente. Si no, se llama a la acción "Guardar
// como...".
void MainWindow::on_actionGuardar_triggered()
{
  if(filename.length() == 0)
    on_actionGuardar_como_triggered();
  else
    save();
}

// Pide al usuario un nombre de archivo antes de realizar el guardado.
void MainWindow::on_actionGuardar_como_triggered()
{
  filename = QFileDialog::getSaveFileName(this, "Guardar como...", "",
                                          "Archivos de jardín (*.garden)");
  if(filename.length() > 0)
    save();
}

// Se pregunta al usuario antes que si quiere guardar el mapa actual y si no
// cancela la operación, elimina todos los obstáculos del mapa.
void MainWindow::on_actionNuevo_triggered(){
  switch(QMessageBox::question(this, "Atención", "Se perderán todos los cambios no guardados."
                               "¿Deseas guardar el jardín actual?",
                               QMessageBox::Ok | QMessageBox::No | QMessageBox::Cancel,
                               QMessageBox::Cancel)){
  case QMessageBox::Ok:
    on_actionGuardar_triggered();
    if(filename.isEmpty())
      return;
    break;
  case QMessageBox::Cancel:
    return;
  case QMessageBox::No:
  default:
    break;
  }

  int iteraciones = 0;
  progressBar->setEnabled(true);

  for(int i = 0; i < rows; ++i){
    for(int j = 0; j < columns; ++j){
      set_pos(i, j, CESPED_A);
      ++iteraciones;
    }
    progressBar->setValue(iteraciones*100/(rows*columns));
  }
  set_pos(0, 0, INICIO);

  if(ini_x != -1)
    set_pos(ini_y, ini_x, PUNTO_A);
  if(fin_x != -1)
    set_pos(fin_y, fin_x, PUNTO_B);

  progressBar->setEnabled(false);
}

void MainWindow::on_actionSalir_triggered(){
  close();
}

// Bloquea todos los controles de la interfaz que causan o pueden causar un
// cambio en el contenido del jardín para que no sucedan hechos extraños
// producidos por el usuario cuando se está simulando.
void MainWindow::lock_interface(bool b){
  ui->cbEdicion->setDisabled(b);
  ui->bCamino->setDisabled(b);
  ui->bSimular->setDisabled(b);
  ui->bReset->setDisabled(b);
  ui->bPruebas->setDisabled(b);
  ui->actionAbrir->setDisabled(b);
  ui->actionGuardar->setDisabled(b);
  ui->actionGuardar_como->setDisabled(b);
  ui->actionSalir->setDisabled(b);
}
