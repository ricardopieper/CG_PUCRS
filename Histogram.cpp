// **********************************************************************
// PUCRS/FACIN
// Programa de testes para manipulação de Imagens
//
// Marcio Sarroglia Pinho
//
// pinho@pucrs.br
// **********************************************************************
//
//  Para rodar no XCODE, veja o arquivo "InstrucoesXCODE.rtf"
//

#include <iostream>
#include <cmath>
#include <thread>
#include <vector>
#include <algorithm>
#include <functional>
#include <GL/glut.h>
#include <GL/freeglut.h>

using namespace std;

#ifdef __APPLE__
#include <GLUT/glut.h>
#endif

#include "ImageClass.h"
ImageClass Image, HistogramImage, ImageMedianNoiseFilter;

#include "Temporizador.h"
Temporizador T;

const int LIMIAR = 100;
#define LARGURA_JAN 1000
#define ALTURA_JAN 500
std::vector<int> peaks;
int peakWidth = 10;
int histogramData[255];
int histogramFiltered[255];
int xLastClick = -1;
int yLastClick = -1;
int window = 5;
int rectRegion = 50;


void IteratePixelsWindow(ImageClass& source, int windowSize, std::function<void(int, int, int)> iteratorFunction) {

    if (windowSize % 2 == 0) {
        windowSize++;
    }

    int neighboringRegion = windowSize / 2;
   
    for (int x=neighboringRegion; x < source.SizeX() - neighboringRegion; x++) {
        for (int y=neighboringRegion; y < source.SizeY() - neighboringRegion; y++) {

            iteratorFunction(x, y, neighboringRegion);
        }
    }

}

void ComputeHistogram(ImageClass img, int window, int p1x, int p1y, int p2x, int p2y) {
    for (int i=0; i < 255; i++) {
        histogramData[i] = 0;
    }
    for (int x = p1x; x < p2x; x++) {
        for (int y = p1y; y < p2y; y++) {
            int intensity = img.GetPointIntensity(x, y);
            if (intensity > 3){
                histogramData[intensity]++;
            }    
        }
    }
}

void ComputeHistogramMedian(int window) {
    int neighboringRegion = window / 2;

    for (int i=0; i < 255; i++) {
        histogramFiltered[i] = 0;
    }

    for (int x=neighboringRegion; x < 255 - neighboringRegion; x++) {
        std::vector<int> values;
        for (int i = 0; i < window; i++) {
            int val = histogramData[(x + i) - neighboringRegion];
            if (val != 0){
                values.push_back(val);
            }
        }
        if (values.size() > 0) {
            std::sort(values.begin(), values.end());
            int mid = values.size() / 2;
            int median = values[mid];
            histogramFiltered[x] = median;
        }
    }
}

void ComputePeaks() {
    peaks.clear();
    int currentCandidate = -1;
    int currentCandidateVal = 0;
    int startedGoingDownIdx = -1;
    int startedGoingUpIdx = -1;
    for (int x = 1 + peakWidth / 2; x < 255; x++) {

        int prev = histogramFiltered[x - 1];
        int cur =  histogramFiltered[x];
        
        if ((startedGoingDownIdx == -1 && cur > prev) || (cur > currentCandidateVal)) {
            //is going up
            currentCandidate = x;
            currentCandidateVal = cur;
            startedGoingDownIdx = -1;
            startedGoingUpIdx = x;
        } else {
            
            if (currentCandidate == -1) {
                continue;
            }

            if (startedGoingDownIdx == -1) {
                startedGoingDownIdx = x;
            }

            if (x - startedGoingDownIdx > peakWidth && startedGoingUpIdx != -1) {
                peaks.push_back(currentCandidate);
                //x = currentCandidate + 1;
                currentCandidate = -1;
                currentCandidateVal = 0x7fffffff;
                startedGoingDownIdx = -1;
                startedGoingUpIdx = -1;
                
            }
        }
    }

    /*if (currentCandidate && startedGoingDownIdx) {
        peaks.push_back(currentCandidate);
    }*/
}


void loadImage() {
    int r = Image.Load("Fatias/fatia02.bmp");

    if (!r) {
        exit(1);
    } 
    else {
        cout << ("Imagem carregada!\n");
    }
   
}

void ComputeWholeImage(int window) {
    ComputeHistogram(Image, window, 0, 0, Image.SizeX(), Image.SizeY());
    ComputeHistogramMedian(window);
    ComputePeaks();
}


void ComputeImageRegion(int window) {

    //centerPoint = xLastClick, yLastClick
    loadImage();
    int p1x = xLastClick - (rectRegion / 2);
    int p2x = xLastClick + (rectRegion / 2);

    int p1y = yLastClick - (rectRegion / 2);
    int p2y = yLastClick + (rectRegion / 2);

    Image.DrawBox(p1x, p1y, p2x, p2y, 255, 0, 0);

    ComputeHistogram(Image, window, p1x, p1y, p2x, p2y);
    ComputeHistogramMedian(window);
    ComputePeaks();
}

void init()
{
    loadImage();

    cout << "Nova Imagem Criada" << endl;
  
    HistogramImage.SetSize(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT), 3);
    ImageMedianNoiseFilter.SetSize(Image.SizeX(), Image.SizeY(), Image.Channels());
    Image.CopyTo(&ImageMedianNoiseFilter);
    
    ComputeWholeImage(window);
}



// **********************************************************************
//  void reshape( int w, int h )
//  trata o redimensionamento da janela OpenGL
// **********************************************************************
void reshape(int w, int h)
{
    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);
    gluOrtho2D(0, w, 0, h);

    // Set the clipping volume
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    HistogramImage.SetSize(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT), 3);
}

void RenderString(int x, int y, void *font, const unsigned char* string, GLfloat r, GLfloat g, GLfloat b) {
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glRasterPos2i(x, y);
    glColor3f( r,g,b );
    glutBitmapString(font, (const unsigned char*)string);
}

void DrawGraphNumbers(int posX, int posY, int width, int height) {
    double pixelXToHistogram = (double)width / 255.0;

     //draw values
    for (int i=10; i < 255; i+=10) {
        auto xPos = posX + (pixelXToHistogram * (i + 1)); 
        const unsigned char* t = (const unsigned char *)(std::to_string(i).c_str());
        RenderString(xPos, 10, GLUT_BITMAP_TIMES_ROMAN_10, t, 0 ,0, 0);
    }
}

void HistogramCalculateAndDraw(int posX, int posY, int width, int height) {

    int totalWidth = glutGet(GLUT_WINDOW_WIDTH);
    int totalHeight = glutGet(GLUT_WINDOW_HEIGHT);

    HistogramImage.DrawLine(posX, posY, posX + width, posY, 255,0,0); 
    HistogramImage.DrawLine(posX, posY, posX, height, 255,0,0); 

    int largestHistogramValue = 0;

    for (int hv: histogramFiltered) {
        if (largestHistogramValue < hv) {
            largestHistogramValue = hv;
        }
    }

    double pixelYToHistogram = ((double)height - (double)posY) / (double)largestHistogramValue;
    double pixelXToHistogram = (double)width / 255.0;

    //draw peaks
    int peakHalf = peakWidth / 2;
    for (auto c: peaks) {

        if (c == -1) continue;

        int mid = posX + (pixelXToHistogram * (c + 1));
        int xStart = mid - (peakHalf * pixelXToHistogram);
        int xEnd = mid + (peakHalf * pixelXToHistogram);

        HistogramImage.FillBox(xStart, posY, xEnd, height, 100, 255 ,100);
    }

    if (largestHistogramValue == 0) return;

    //draw values
    for (int i=0; i< 255; i++) {

        auto xPos = posX + (pixelXToHistogram * (i + 1)); 
        auto histogramValue = histogramFiltered[i];
        auto ySize = histogramValue * pixelYToHistogram;
      
        HistogramImage.DrawLine(xPos, posY, xPos, posY + ySize, 0,0,0);
    }

}



// **********************************************************************
//  void display( void )
// **********************************************************************
void display(void)
{
    auto img = Image;
    static double AccumDeltaT = 0;

    double dt = T.getDeltaT();
    AccumDeltaT += dt;
    if (AccumDeltaT > 1) // imprime o frame rate a cada 3 segundos
    {
        AccumDeltaT = 0;
        cout << "FPS: " << 1.0 / dt << endl;
    }
   

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Fundo de tela azul
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    // Ajusta o ZOOM da imagem para que apareca na metade da janela

    float zoomFactor = 1;

    float zoomH = (glutGet(GLUT_WINDOW_WIDTH) / (2.0 * zoomFactor)) / (double)Image.SizeX();
    float zoomV = (glutGet(GLUT_WINDOW_HEIGHT) / zoomFactor) / (double)Image.SizeY();
    img.SetZoomH(zoomH);
    img.SetZoomV(zoomV);
    // posiciona a imagem no canto inferior esquerdo da janela
    img.SetPos(0, 0);

    //HistogramImage.SetSize(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT), 3);
    HistogramImage.SetPos(glutGet(GLUT_WINDOW_WIDTH) / 2, 0);
    HistogramImage.Clear();
    HistogramCalculateAndDraw(20 + glutGet(GLUT_WINDOW_WIDTH) / 2, 20, -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, -20 + glutGet(GLUT_WINDOW_HEIGHT));
       
    // posiciona a imagem nova na metada da direita da janela
    // Ajusta o ZOOM da imagem para que apareca na metade da janela

    // Coloca as imagens na tela
    img.Display();

    if (xLastClick != -1) {
        ComputeImageRegion(window);
    }

    xLastClick = -1;
    yLastClick = -1;

    HistogramImage.Display();
    DrawGraphNumbers(20 + glutGet(GLUT_WINDOW_WIDTH) / 2, 20, -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, -20 + glutGet(GLUT_WINDOW_HEIGHT));
    // Mostra a tela OpenGL
    glutSwapBuffers();

    //std::this_thread::sleep_for(std::chrono::milliseconds(240));
}
// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
// **********************************************************************
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27: // Termina o programa qdo
        exit(0); // a tecla ESC for pressionada
        break;
    default:
        break;
    }
}

// **********************************************************************
//  void arrow_keys ( int a_keys, int x, int y )
// **********************************************************************
void arrow_keys(int a_keys, int x, int y)
{
    switch (a_keys)
    {
    case GLUT_KEY_UP: // When Up Arrow Is Pressed...
        break;
    case GLUT_KEY_DOWN: // When Down Arrow Is Pressed...

        break;
    default:
        break;
    }
}

// **********************************************************************
// **********************************************************************
void Mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
    {
        double windowWidth = glutGet(GLUT_WINDOW_WIDTH);
        double windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

        double windowToImageX = windowWidth / (double)Image.SizeX();
        double windowToImageY = windowHeight / (double)Image.SizeY();

        int imgClickX = x;
        int imgClickY =  Image.SizeY() - y;

        cout << Image.GetPointIntensity(imgClickX, imgClickY) << endl;
        Image.DrawPixel(imgClickX, imgClickY, 255, 0, 0);
        
        xLastClick = imgClickX;
        yLastClick = imgClickY;
        glutPostRedisplay();

    }
}

// **********************************************************************
//  void main ( int argc, char** argv )
// **********************************************************************
int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    glutInitWindowPosition(10, 10);

    // Define o tamanho da janela gráfica do programa
    glutInitWindowSize(LARGURA_JAN, ALTURA_JAN);
    glutCreateWindow("Image Loader");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(arrow_keys);
    glutMouseFunc(Mouse);

    glutIdleFunc(display);

    glutMainLoop();
}
