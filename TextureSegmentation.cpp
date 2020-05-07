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
#include <queue>
#include <thread>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <sys/stat.h>
using namespace std;

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/freeglut.h>
#endif

#include "ImageClass.h"
ImageClass Image, ImageFiltered, ImageSegmented, HistogramImage;

#include "Temporizador.h"
Temporizador T;

#define LARGURA_JAN 1000
#define ALTURA_JAN 500

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::vector<float> histogramData;
std::vector<float> histogramFiltered;

int window = 13;
float minBrightness = -1;
float contrastLevel = -1;
std::vector<int> peaks;
int peakWidth = 10;

void ComputePeaks()
{
    peaks.clear();
    int currentCandidate = -1;
    int currentCandidateVal = 0;
    int startedGoingDownIdx = -1;
    int startedGoingUpIdx = -1;

    auto minThreshold = *std::max_element(histogramFiltered.begin(), histogramFiltered.end()) * 0.1;

    for (int x = 1 + peakWidth / 2; x < 255; x++)
    {

        int prev = histogramFiltered[x - 1];
        int cur = histogramFiltered[x];

        if ((startedGoingDownIdx == -1 && cur > prev) || (cur > currentCandidateVal))
        {
            //is going up
            currentCandidate = x;
            currentCandidateVal = cur;
            startedGoingDownIdx = -1;
            startedGoingUpIdx = x;
        }
        else
        {

            if (currentCandidate == -1)
            {
                continue;
            }

            if (startedGoingDownIdx == -1)
            {
                startedGoingDownIdx = x;
            }

            if (x - startedGoingDownIdx > peakWidth && startedGoingUpIdx != -1)
            {
                if (currentCandidateVal > minThreshold)
                {
                    peaks.push_back(currentCandidate);
                }
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

struct RGB
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

void RenderString(int x, int y, void *font, const unsigned char *string, GLfloat r, GLfloat g, GLfloat b)
{
    glColor3f(r, g, b);
    glRasterPos2i(x, y);
    glutBitmapString(font, (const unsigned char *)string);
}

void DrawGraphNumbers(int step, int domainRange, int posX, int posY, int width, int height, RGB textColor)
{
    double pixelXToDomain = (double)width / domainRange;

    //draw values
    for (int i = step; i < domainRange; i += step)
    {
        auto xPos = posX + (pixelXToDomain * (i + 1));
        const unsigned char *t = (const unsigned char *)(std::to_string(i).c_str());
        RenderString(xPos, posY, GLUT_BITMAP_TIMES_ROMAN_10, t, textColor.R / 255.0f, textColor.G / 255.0, textColor.B / 255.0);
    }
}

void ComputeHistogram()
{
    for (int i = 0; i < 255; i++)
    {
        histogramData[i] = 0;
    }

    for (int i = 0; i < ImageFiltered.SizeX(); i++)
    {
        for (int j = 0; j < ImageFiltered.SizeY(); j++)
        {
            int intensity = (int)ImageFiltered.GetPointIntensity(i, j);
            if (intensity > 5)
            {
                histogramData[intensity]++;
            }
        }
    }
}

void SegmentImage()
{

    int neighboringRegion = window / 2;

    for (int i = 0; i < ImageSegmented.SizeX(); i++)
    {
        for (int j = 0; j < ImageSegmented.SizeY(); j++)
        {
            ImageSegmented.DrawPixel(i, j, 0, 0, 0);
        }
    }

    for (int i = 0; i < ImageFiltered.SizeX(); i++)
    {
        for (int j = 0; j < ImageFiltered.SizeY(); j++)
        {
            int intensity = (int)ImageFiltered.GetPointIntensity(i, j);

            /*25 -> 45:  255, 0, 0
              45 -> 85:  0, 255, 0
              85 -> 105: 0, 0, 255*/
            if (intensity > 1 && intensity < 25)
            {
                ImageSegmented.DrawPixel(i + neighboringRegion, j + neighboringRegion, 255, 0, 0);
            }
            else if (intensity >= 45 && intensity <= 85)
            {
                ImageSegmented.DrawPixel(i + neighboringRegion, j + neighboringRegion, 0, 255, 0);
            }
            else if (intensity > 220)
            {
                ImageSegmented.DrawPixel(i + neighboringRegion, j + neighboringRegion, 0, 0, 255);
            }
            else
            {
                ImageSegmented.DrawPixel(i + neighboringRegion, j + neighboringRegion, 0, 0, 0);
            }
        }
    }
}

void HistogramCalculateAndDraw(int posX, int posY, int width, int height)
{

    int totalWidth = glutGet(GLUT_WINDOW_WIDTH);
    int totalHeight = glutGet(GLUT_WINDOW_HEIGHT);

    HistogramImage.DrawLine(posX, posY, posX + width, posY, 255, 0, 0);
    HistogramImage.DrawLine(posX, posY, posX, posY + height, 255, 0, 0);

    float largestHistogramValue = *std::max_element(histogramFiltered.begin(), histogramFiltered.end());
    if (largestHistogramValue == 0)
        return;

    double pixelYToHistogram = (double)height / (double)largestHistogramValue;
    double pixelXToHistogram = (double)width / 255.0f;

    //draw peaks
    int peakHalf = peakWidth / 2;
    for (auto c : peaks)
    {

        if (c == -1)
            continue;

        int mid = posX + (pixelXToHistogram * (c + 1));
        int xStart = mid - (peakHalf * pixelXToHistogram);
        int xEnd = mid + (peakHalf * pixelXToHistogram);

        HistogramImage.FillBox(xStart, posY, xEnd, height, 100, 255, 100);
    }

    //draw values
    for (int i = 0; i < 255; i++)
    {

        auto xPos = (double)posX + (pixelXToHistogram * (i + 1));
        auto histogramValue = histogramFiltered[i];
        auto ySize = histogramValue * pixelYToHistogram;

        HistogramImage.DrawLine(xPos, posY, xPos, posY + ySize, 0, 0, 0);
    }
}

struct Pixel
{
    int x;
    int y;
    float value;
};

std::vector<Pixel> pixels;

void ReloadImage(std::string path)
{
    Image.Load(path.c_str());
}

void ComputeHistogramMedian(int window)
{
    int neighboringRegion = window / 2;

    for (int i = 0; i < 255; i++)
    {
        histogramFiltered[i] = 0;
    }

    for (int x = neighboringRegion; x < 255 - neighboringRegion; x++)
    {
        std::vector<int> values;
        for (int i = 0; i < window; i++)
        {
            int val = histogramData[(x + i) - neighboringRegion];
            if (val != 0)
            {
                values.push_back(val);
            }
        }
        if (values.size() > 0)
        {
            std::sort(values.begin(), values.end());
            int mid = values.size() / 2;
            int median = values[mid];
            histogramFiltered[x] = median;
        }
    }
}

void ComputeWholeImage(int window)
{
    histogramFiltered.clear();
    HistogramImage.Clear();

    for (int i = 0; i < 255; i++)
    {
        histogramFiltered.push_back(0);
        histogramData.push_back(0);
    }

    ComputeHistogram();
    ComputeHistogramMedian(window);
}

void ComputeImage(std::string path, std::string resultPath, std::string gabarito)
{
    ReloadImage(path);

    ImageClass imgGabarito;
    imgGabarito.Load(gabarito.c_str());

    if (window % 2 == 0)
    {
        window++;
    }

    int neighboringRegion = window / 2;

    ImageFiltered.SetSize(Image.SizeX() - neighboringRegion, Image.SizeY() - neighboringRegion, 3);

    ImageSegmented.SetSize(Image.SizeX(), Image.SizeY(), 3);

    int threads = 4;

    int widthPerThread = Image.SizeX() / threads;
    int heightPerThread = Image.SizeY() / threads;

    std::vector<std::thread> processingThreads;

    for (int threadNum = 0; threadNum < threads; threadNum++)
    {

        processingThreads.push_back(std::thread([threads, threadNum, neighboringRegion, widthPerThread, heightPerThread]() {
            std::vector<double> intensities;
            int regionStart = 0;
            int regionEnd = 0;

            if (threadNum == 0)
            {
                regionStart = neighboringRegion;
            }

            if (threadNum == (threads - 1))
            {
                regionEnd = neighboringRegion;
            }

            for (int x = regionStart + (widthPerThread * threadNum);
                 x < -regionEnd + (widthPerThread * (threadNum + 1));
                 x++)
            {

                for (int y = neighboringRegion;
                     y < -neighboringRegion + Image.SizeY();
                     y++)
                {

                    intensities.clear();
                    for (int i = 0; i < window; i++)
                    {
                        for (int j = 0; j < window; j++)
                        {

                            int iWindow = i + x - neighboringRegion;
                            int jWindow = j + y - neighboringRegion;

                            intensities.push_back(Image.GetPointIntensity(iWindow, jWindow));
                        }
                    }
                    std::sort(intensities.begin(), intensities.end());
                    auto midpoint = intensities.size() / 2;
                    ImageFiltered.SetPointIntensity(x - neighboringRegion, y - neighboringRegion, intensities[midpoint]);
                }
            }
        }));
    }

    for (int threadNum = 0; threadNum < threads; threadNum++)
    {
        processingThreads[threadNum].join();
    }

    ComputeWholeImage(window);
    ComputePeaks();
    SegmentImage();
    //HistogramImage.SetSize(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT), 3);
    // auto dilated = dilate_1(ImageSegmented, RGB { 255, 0, 0 },  RGB { 0, 0, 0 });

    // auto eroded = erode_1(dilated, RGB { 0, 255, 0 },  RGB { 255, 0, 0 });

    ImageSegmented.Save(resultPath.c_str());

    int truePositive = 0;
    int falsePositive = 0;

    for (int i = 0; i < ImageSegmented.SizeX(); i++)
    {
        for (int j = 0; j < ImageSegmented.SizeY(); j++)
        {

            unsigned char r, g, b;
            unsigned char gr, gg, gb;

            ImageSegmented.ReadPixel(i, j, r, g, b);
            imgGabarito.ReadPixel(i, j, gr, gg, gb);

            if (r == gr && g == gg && b == gb)
            {
                truePositive++;
            }
            else
            {
                falsePositive++;
            }
        }
    }

    std::cout << "True Positive: " << truePositive << std::endl;
    std::cout << "False Positive: " << falsePositive << std::endl;
    std::cout << "Accuracy: " << (float)truePositive / (float)(truePositive + falsePositive) << std::endl;
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
    auto img = ImageSegmented;
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

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    HistogramImage.SetPos(glutGet(GLUT_WINDOW_WIDTH) / 2, 0);
    HistogramImage.Clear();

    HistogramCalculateAndDraw(
        20 + glutGet(GLUT_WINDOW_WIDTH) / 2,
        20,
        -40 + glutGet(GLUT_WINDOW_WIDTH) / 2,
        -40 + (glutGet(GLUT_WINDOW_HEIGHT) / 1));

    HistogramImage.Display();

    DrawGraphNumbers(10, 255, 20 + glutGet(GLUT_WINDOW_WIDTH) / 2, 10, -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, -20 + glutGet(GLUT_WINDOW_HEIGHT),
                     RGB{0, 0, 0});

    //DrawGraphNumbers(10, 255, 20 + glutGet(GLUT_WINDOW_WIDTH) / 2,  10 + (glutGet(GLUT_WINDOW_HEIGHT) / 2), -40 + glutGet(GLUT_WINDOW_WIDTH) / 2, -20 + glutGet(GLUT_WINDOW_HEIGHT),
    //    RGB{0,0,0});

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
    case 27:     // Termina o programa qdo
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

struct Part {
    int quantity;
    std::string name;

    bool operator<(const Part &d) const
    {
    	return d.quantity < quantity;
    } 
    
    bool operator>(const Part &d) const
    {
    	return d.quantity > quantity;
    }  
};

void CropImage(int num,
               std::string image,
               std::string truth,
               std::string folder)
{
    std::string cropFolder = folder + "/" + std::to_string(num);
    mkdir(cropFolder.c_str(), 0777);

    ImageClass img;
    img.Load(image.c_str());
    
    ImageClass gt;
    gt.Load(truth.c_str());

    int size = 20;

    for (int i = 0; i < img.SizeX(); i += size)
    {
        for (int j = 0; j < img.SizeY(); j += size)
        {

            ImageClass cropped;
            cropped.SetSize(size, size, 3);

            int tons = 32;
            double tons_mult = 255.0f / (double)tons;

            int dentina = 0;
            int canal = 0;
            int fundo = 0;
            int pino = 0;

            for (int x = 0; x < size; x++)
            {
                for (int y = 0; y < size; y++)
                {
                    int _x = i + x;
                    int _y = j + y;
                    
                    auto intensity = img.GetPointIntensity(_x, _y);
                    intensity = (int)(intensity / tons_mult);

                    cropped.SetPointIntensity(x, y, (int)(intensity * tons_mult));
                  //  cropped.DrawPixel(x, y, r, g, b);

                    int r = gt.ReadR(_x, _y);
                    int g = gt.ReadG(_x, _y);
                    int b = gt.ReadB(_x, _y);
                    
                    
                    if (r == 255)
                    {
                        canal++;
                    }
                    else if (g == 255)
                    {
                        dentina++;
                    }
                    else if (b == 255)
                    {
                        pino++;
                    }
                    else
                    {
                        fundo++;
                    }
                }
            }

            if (dentina == 0 && canal == 0 && pino == 0)
            {
                //se for 100% fundo: ignora
                continue;
            }

            std::string result = cropFolder + "/crop_" + std::to_string(i) + "_" + std::to_string(j) + ".bmp";
          
            cropped.Save(result.c_str());
          
            std::cout << "Cropped " << num << " " << i << " " << j << std::endl;

            std::string cropDescription = cropFolder + "/crop_" + std::to_string(i) + "_" + std::to_string(j) + ".txt";

            std::priority_queue<Part, std::vector<Part>, std::greater<Part>> parts;

            parts.push(Part { .quantity = dentina, .name = "dentina" });
            parts.push(Part { .quantity = canal, .name = "canal" });
            parts.push(Part { .quantity = pino, .name = "pino" });
            parts.push(Part { .quantity = fundo, .name = "findo" });

            auto largest = parts.top();
          
            ofstream s;
            s.open(cropDescription.c_str());
            s << "TYPE=" << largest.name << std::endl;
            s.close();
        }
    }
}

// **********************************************************************
//  void main ( int argc, char** argv )
// **********************************************************************
int main(int argc, char **argv)
{
    /* glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    glutInitWindowPosition(10, 10);

    // Define o tamanho da janela gráfica do programa
    glutInitWindowSize(LARGURA_JAN, ALTURA_JAN);
    glutCreateWindow("Image Loader"); */

    for (int i = 727; i <= 731; i++)
    {
        CropImage(i, "/home/ricardo/Downloads/Dados9/" + std::to_string(i) + ".bmp",
                  "/home/ricardo/Downloads/Dados9/Editados/bmp/A2 Dentes 01 02 03__rec0" + std::to_string(i) + ".bmp",
                  "/home/ricardo/Downloads/Dados9/crops");

        //ComputeImage(
        //    "/home/ricardo/Downloads/Dados9/"+std::to_string(i)+".bmp",
        //   "/home/ricardo/Downloads/Dados9/Resultados/"+std::to_string(i)+".bmp",
        //   "/home/ricardo/Downloads/Dados9/Editados/bmp/"+std::to_string(i)+".bmp");
    }

    /*
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(arrow_keys);
    glutMouseFunc(Mouse);

    glutIdleFunc(display);

    glutMainLoop(); */
}
