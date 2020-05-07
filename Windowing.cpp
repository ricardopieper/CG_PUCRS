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


using namespace std;

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/freeglut.h>
#endif

#include "ImageClass.h"
ImageClass Image, HistogramImage;

#include "Temporizador.h"
Temporizador T;

#define LARGURA_JAN 1000
#define ALTURA_JAN 500

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

float minval = -1;
float maxval = -1;
int rows = 0;
int cols = 0;
std::vector<float> histogramData;
std::vector<float> histogramFiltered;
std::vector<float> inWindowValues;
std::vector<float> inWindowPixelsRescaled;
std::vector<float> inWindowHistogramData;

int window = 25;
float minBrightness = -1;
float contrastLevel = -1;
vector<float> originalData;

struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
};



void RenderString(int x, int y, void *font, const unsigned char* string, GLfloat r, GLfloat g, GLfloat b) {
    glColor3f( r,g,b );
    glRasterPos2i(x, y);
    glutBitmapString(font, (const unsigned char*)string);
}

void DrawGraphNumbers(int step, int domainRange, int posX, int posY, int width, int height, RGB textColor) {
    double pixelXToDomain = (double)width / domainRange;

     //draw values
    for (int i=step; i < domainRange; i+=step) {
        auto xPos = posX + (pixelXToDomain * (i + 1)); 
        const unsigned char* t = (const unsigned char *)(std::to_string(i).c_str());
        RenderString(xPos, posY, GLUT_BITMAP_TIMES_ROMAN_10, t, textColor.R/255.0f ,textColor.G/255.0, textColor.B / 255.0);
    }
}

void ComputeHistogram() {
    for (int i=0; i < maxval; i++) {
        histogramData[i] = 0;
    }

    for (int i=0; i< 255; i++) {
        inWindowHistogramData[i] = 0;
    }

    for (float f: inWindowValues) {
        int intensity = (int)f;
        if (intensity > 3){
            histogramData[intensity]++;
        }    
    }


    for (float f: inWindowPixelsRescaled) {
        int intensity = (int)f;
        if (intensity > 3){
            inWindowHistogramData[intensity]++;
        }    
    }
}

template <typename T>
void DrawHistogram(std::vector<T> histData, T domainRange, int posX, int posY, int width, int height, RGB lineColor) {

    int totalWidth = glutGet(GLUT_WINDOW_WIDTH);
    int totalHeight = glutGet(GLUT_WINDOW_HEIGHT);

    HistogramImage.DrawLine(posX, posY, posX + width, posY, 255,0,0); 
    HistogramImage.DrawLine(posX, posY, posX, posY + height, 255,0,0); 

    T largestHistogramValue = *std::max_element(histData.begin(), histData.end());
    if (largestHistogramValue == 0) return;

    double pixelYToHistogram = (double)height / (double)largestHistogramValue;
    double pixelXToHistogram = (double)width / domainRange;

    //draw values
    for (T i=0; i< domainRange; i++) {

        auto xPos = (double)posX + (pixelXToHistogram * (i + 1)); 
        auto histogramValue = histData[i];
        auto ySize = histogramValue * pixelYToHistogram;
      
        HistogramImage.DrawLine(xPos, posY, xPos, posY + ySize, lineColor.R,lineColor.G, lineColor.B);
    }

}

void HistogramCalculateAndDraw(int posX, int posY, int width, int height) {
    DrawHistogram(histogramFiltered, maxval, posX, posY, width, height, RGB{0, 0, 0});
}


//https://github.com/ricardopieper/fdeb-rs/blob/master/src/fdeb_utils.rs
float rescale(float value, float sourceMin, float sourceMax,
    float targetMin, float targetMax) {
    float sourceRange = sourceMax - sourceMin;
    float targetRange = targetMax - targetMin;
    float offsetInSource = value - sourceMin;
    return targetMin + ((offsetInSource / sourceRange) * targetRange);
}

struct Pixel {
    int x;
    int y;
    float value;
};

std::vector<Pixel> pixels;

void ReloadImage() {
    Image.SetSize(rows, cols, 3);

    float rangeStart = minBrightness;
    float rangeEnd = rangeStart + contrastLevel;
    inWindowValues.clear();
    inWindowPixelsRescaled.clear();

    pixels.clear();

    for (int i = 0; i < originalData.size(); i++) {

        auto x = i % rows; 
        auto y = cols - (i / cols); 

        float value = originalData[i];
        pixels.push_back(Pixel{x, y, value});
        if (value > rangeEnd || value < rangeStart) {
        } else {
            inWindowValues.push_back(value);
        }
    }


    auto minmax = std::minmax_element(inWindowValues.begin(), inWindowValues.end());

    for (auto pixel: pixels) {
        if (pixel.value > rangeEnd) {
            Image.SetPointIntensity(pixel.x, pixel.y, 255);
        }
         else if (pixel.value < rangeStart) {
            Image.SetPointIntensity(pixel.x, pixel.y, 0);
        } else {
            float rescaled = rescale(
                pixel.value, *minmax.first, *minmax.second, 0, 255
            );
            inWindowPixelsRescaled.push_back(rescaled);
            Image.SetPointIntensity(pixel.x, pixel.y, rescaled);
        }
    }

    //render controls

    float startAt = rescale(minval, minval, maxval, 150, Image.SizeY() - 20);
    float finishAt = rescale(maxval, minval, maxval, 150, Image.SizeY() - 20);

    float selectedStartAt = rescale(rangeStart, minval, maxval, 150, Image.SizeY() - 20);
    float selectedFinishAt = rescale(rangeEnd, minval, maxval, 150, Image.SizeY() - 20);

    Image.DrawLine(10, startAt, 10, finishAt, 0, 255, 0);
    Image.DrawLine(20, selectedStartAt, 20, selectedFinishAt, 100, 150, 255);
}

void ComputeHistogramMedian(int window) {
    int neighboringRegion = window / 2;

    for (int i=0; i < maxval; i++) {
        histogramFiltered[i] = 0;
    }

    for (int x=neighboringRegion; x < maxval - neighboringRegion; x++) {
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

void ComputeWholeImage(int window) {
    histogramFiltered.clear();
    HistogramImage.Clear();
    inWindowHistogramData.clear();

    for (int i=0; i< maxval; i++) {
        histogramFiltered.push_back(0);
        histogramData.push_back(0);
    }

    for (int i=0; i< 255; i++) {
        inWindowHistogramData.push_back(0);
    }

    ReloadImage();
    ComputeHistogram();
    ComputeHistogramMedian(window);
}

void init()
{
    ifstream infile("exame_sem_cranio.txt");
  
    while (infile)
    {
        string s;
        if (!getline( infile, s )) break;
        istringstream ss( s );
        cols = 0;
        while (ss)
        {
            string s;
            if (!getline( ss, s, '\t')) {
                break;
            }
            originalData.push_back( std::stof(s) );
            cols++;
        }
        rows++;
    }
    if (!infile.eof())
    {
        cerr << "Nao abriu o arquivo!\n";
    }

    auto minmax = std::minmax_element(originalData.begin(), originalData.end());
    minval = *minmax.first;
    maxval = *minmax.second;

    minBrightness = 0;
    contrastLevel = maxval;

    ReloadImage();
    ComputeWholeImage(window);
    HistogramImage.SetSize(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT), 3);


    auto minmax_hist = std::minmax_element(histogramData.begin(), histogramData.end());
    auto minmax_hist2 = std::minmax_element(histogramFiltered.begin(), histogramFiltered.end());
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
   

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Fundo de tela preto
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    // Ajusta o ZOOM da imagem para que apareca na metade da janela

    float zoomFactor = 1;
    
    // posiciona a imagem nova na metada da direita da janela
    // Ajusta o ZOOM da imagem para que apareca na metade da janela

    float zoomH = (glutGet(GLUT_WINDOW_WIDTH) / (2 * zoomFactor)) / (double)Image.SizeX();
    float zoomV = (glutGet(GLUT_WINDOW_HEIGHT) / zoomFactor) / (double)Image.SizeY();
    img.SetZoomH(zoomH);
    img.SetZoomV(zoomV);
    // posiciona a imagem no canto inferior esquerdo da janela
    img.SetPos(0, 0);

   
    
    // Coloca as imagens na tela
    img.Display();

    std::string min = "Min value = "+std::to_string(minval);
    std::string max = "Max value = "+std::to_string(maxval);
    std::string contrast = "Contrast = "+std::to_string(contrastLevel);
    std::string brightness = "Brightness = "+std::to_string(minBrightness);
    std::string fullWindow = "Green is the full pixel range";
    std::string selectedWindow = "Light blue is the selected pixel range";
    std::string instructions = "Up and Down arrow keys to adjust brightness, Left and Right to adjust contrast";

    glMatrixMode(GL_MODELVIEW );
    glLoadIdentity();
    RenderString(40, glutGet(GLUT_WINDOW_HEIGHT) - 15, GLUT_BITMAP_HELVETICA_12,(unsigned char *)instructions.c_str(), 1.0f, 1.0f, 1.0f);
    RenderString(40, glutGet(GLUT_WINDOW_HEIGHT) - 30, GLUT_BITMAP_HELVETICA_12, (unsigned char *)selectedWindow.c_str(), 100.0f/255.0f, 150.0f/255.0f, 1.0f);
    RenderString(40, glutGet(GLUT_WINDOW_HEIGHT) - 45, GLUT_BITMAP_HELVETICA_12, (unsigned char *)fullWindow.c_str(), 0.0f, 1.0f, 0.0f);
    RenderString(40, 55, GLUT_BITMAP_HELVETICA_12, (unsigned char *)brightness.c_str(), 1.0f, 0, 0);
    RenderString(40, 40, GLUT_BITMAP_HELVETICA_12, (unsigned char *)contrast.c_str(), 1.0f, 0, 0);
    RenderString(40, 25, GLUT_BITMAP_HELVETICA_12, (unsigned char *)min.c_str(), 1.0f, 0, 0);
    RenderString(40, 10, GLUT_BITMAP_HELVETICA_12, (unsigned char *)max.c_str(), 1.0f, 0, 0);

    HistogramImage.SetPos(glutGet(GLUT_WINDOW_WIDTH) / 2, 0);
    HistogramImage.Clear();
    
    DrawHistogram(inWindowHistogramData, 255.0f, 
        20 + glutGet(GLUT_WINDOW_WIDTH) / 2, 
        20 + (glutGet(GLUT_WINDOW_HEIGHT) / 2),
        -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, 
        -40 + (glutGet(GLUT_WINDOW_HEIGHT) / 2),
        RGB{50,50,50});

    HistogramCalculateAndDraw(
        20 + glutGet(GLUT_WINDOW_WIDTH) / 2, 
        20, 
        -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, 
        -20 + (glutGet(GLUT_WINDOW_HEIGHT) / 2));


    HistogramImage.Display();
    
    DrawGraphNumbers(300, maxval, 20 + glutGet(GLUT_WINDOW_WIDTH) / 2, 10, -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, -20 + glutGet(GLUT_WINDOW_HEIGHT),
        RGB{0,0,0});

    DrawGraphNumbers(10, 255, 20 + glutGet(GLUT_WINDOW_WIDTH) / 2,  10 + (glutGet(GLUT_WINDOW_HEIGHT) / 2), -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, -20 + glutGet(GLUT_WINDOW_HEIGHT),
        RGB{0,0,0});

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
        minBrightness += 50;
        break;
    case GLUT_KEY_DOWN: // When Down Arrow Is Pressed...
        minBrightness -= 50;
        break;
    case GLUT_KEY_LEFT:
        contrastLevel -= 30;
        break;
    case GLUT_KEY_RIGHT:
        contrastLevel += 30;
        break;
    default:
        break;
    }
    ComputeWholeImage(window);
}

// **********************************************************************
// **********************************************************************
void Mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
    {
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
