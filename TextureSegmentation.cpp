//#define DEBUG_VIEW

#include <iostream>
#include <cmath>
#include <thread>
#include <vector>
#include <algorithm>
#include <functional>
#include <queue>
#include <thread>
#include <sys/stat.h>

#include "ImageClass.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "linq/core.hpp"
#include "linq/query.hpp"
#include "linq/aggregate.hpp"
#include "linq/to_container.hpp"
#include <string>
#include <iostream>
#include <dirent.h>
#include <tuple>

using namespace std;
using namespace linq;

const int tons = 16;
const int cropSize = 20;

struct MCOStatistics
{
#ifdef DEBUG_VIEW
    ImageClass visualization;
#endif
    double energy;
    double entropy;
    double contrast;
    double variance;
    double homogeinity;
};

double Sum(int width, int height, std::function<double(int, int)> function)
{
    double result = 0;
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            result += function(i, j);
        }
    }
    return isnan(result)? 0 : result;
}

template <int tons>
MCOStatistics MCO(ImageClass img, int cropStartX, int cropStartY, int dx, int dy)
{

    int n[tons][tons];
#ifdef DEBUG_VIEW
    ImageClass img2;
    img2.SetSize(tons, tons, 3);
#endif

    //Zera a matriz
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < tons; j++)
        {
            n[i][j] = 0;
#ifdef DEBUG_VIEW
            img2.SetPointIntensity(i, j, 0);
#endif
        }
    }

    //Calcula MCO
    for (int i = cropStartX; i < cropStartX + cropSize; i++)
    {
        for (int j = cropStartY; j < cropStartY + cropSize; j++)
        {

            bool insideLimits = (i + dx) < img.SizeX() && (i + dx) >= 0 && (j + dy) < img.SizeY() && (j + dy) >= 0;

            if (insideLimits)
            {
                auto px1 = img.GetPointIntensityRounded(i, j);
                auto px2 = img.GetPointIntensityRounded(i + dx, j + dy);
                n[(int)px1][(int)px2]++;
            }
        }
    }

    //Encontra o maior
    int largest = 0;
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < tons; j++)
        {
            if (largest < n[i][j])
            {
                largest = n[i][j];
            }
            //img.SetPointIntensity(i, j, n[i][j]);
        }
    }

    //normaliza no intervalo 0 .. 1 e renderiza img
    double normalized[tons][tons];
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < tons; j++)
        {
            normalized[i][j] = ((double)n[i][j]) / (double)largest;
        }
    }
#ifdef DEBUG_VIEW
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < tons; j++)
        {
            img2.SetPointIntensity(i, j, normalized[i][j] * 255);
        }
    }
#endif

    double energy = Sum(tons, tons, [n](int x, int y) {
        return n[x][y] * n[x][y];
    });

    double contrast = Sum(tons, tons, [n](int x, int y) {
        return (double)n[x][y] * pow((double)x - y, 2.0f);
    });

    double entropy = -1.0f * Sum(tons, tons, [n](int x, int y) {
        if (n[x][y] > 0)
        {
            return n[x][y] * log2(n[x][y]);
        }
        else
        {
            return (double)0;
        }
    });

    double variance = Sum(tons, tons, [n](int x, int y) {
        return (double)n[x][y] * (double)abs(x - y);
    });

    double homogeinity = Sum(tons, tons, [n](int x, int y) {
        double val = (double)n[x][y];
        return val / (1.0 + pow(x - y, 2.0f));
    });

    return MCOStatistics{
#ifdef DEBUG_VIEW
        .visualization = img2,
#endif
        .energy = energy,
        .entropy = entropy,
        .contrast = contrast,
        .variance = variance,
        .homogeinity = homogeinity,
    };
}

struct MCCStatistics
{
#ifdef DEBUG_VIEW
    ImageClass visualization;
#endif
    double runPercentage;
    double shortRunEmphasis;
    double longRunEmphasis;
    double greyLevelNonUniformity;
    double runLengthNonUniformity;
    double lowGreyLevelRunEmphasis;
    double highGreyLevelRunEmphasis;
    double shortRunLowGreyLevelEmphasis;
    double shortRunHighGreyLevelEmphasis;
    double longRunLowGreyLevelEmphasis;
    double longRunHighGreyLevelEmphasis;
};

template <int tons, int sizex, int sizey>
MCCStatistics MCC(int img[cropSize][cropSize])
{

    constexpr int maxRunLength = tons;

    int rlm[tons][maxRunLength];
#ifdef DEBUG_VIEW
    ImageClass img2;
    img2.SetSize(tons, maxRunLength, 3);
#endif
    //zera a rlm (mcc)
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < maxRunLength; j++)
        {
            rlm[i][j] = 0;

#ifdef DEBUG_VIEW
            img2.SetPointIntensity(i, j, 0);
#endif
        }
    }

    //calcula a rlm (direcao horizontal)
    for (int i = 0; i < cropSize; i++)
    {
        int currentRunLength = 1;
        int lastGreyLevel = img[i][0];

        for (int j = 1; j < cropSize; j++)
        {
            bool isSamePixel = lastGreyLevel == img[i][j];
            bool isLast = j == cropSize - 1;

            if (isSamePixel)
            {
                currentRunLength++;
            }

            if ((isLast || !isSamePixel) && currentRunLength > 0)
            {
                rlm[lastGreyLevel][currentRunLength]++;
            }

            if (!isSamePixel)
            {
                currentRunLength = 1;
            }
            lastGreyLevel = img[i][j];
        }
    }

    //Encontra o maior
    int largest = 0;
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < maxRunLength; j++)
        {
            if (largest < rlm[i][j])
            {
                largest = rlm[i][j];
            }
            //img.SetPointIntensity(i, j, n[i][j]);
        }
    }

    //prepara uma visualização

    double normalized[tons][maxRunLength];
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < maxRunLength; j++)
        {
            normalized[i][j] = ((double)rlm[i][j]) / (double)largest;
        }
    }

#ifdef DEBUG_VIEW
    for (int i = 0; i < tons; i++)
    {
        for (int j = 0; j < tons; j++)
        {
            img2.SetPointIntensity(i, j, normalized[i][j] * 255);
        }
    }
#endif

    auto rlmIter = range(0, tons) >> select_many([](int x) { return range(0, maxRunLength) >> select([x](int y) {
                                                                        return std::make_tuple(x, y);
                                                                    }); }, [](auto r, auto it) { return it; });

    double numberOfRuns = Sum(tons, maxRunLength, [rlm](int x, int y) {
        return rlm[x][y];
    });

    double nrInv = 1.0d / numberOfRuns;

    double shortRunEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                  return (double)rlm[i][k] / pow((double)k + 1, 2);
                              });

    double longRunEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                 return (double)rlm[i][k] * pow((double)k + 1, 2);
                             });

    double greyLevelNonUniformity = nrInv * (range(0, tons) >> select([rlm](int i) {
                                                 double s = range(0, maxRunLength) >> select([i, rlm](int j) {
                                                                return (double)rlm[i][j];
                                                            }) >>
                                                            sum();
                                                 s = pow(s, 2);
                                                 return s;
                                             }) >>
                                             sum());

    //a unica diferenca desse aqui pro de cima é que eles navegam em sentidos diferentes. linha x coluna vs coluna x linha
    double runLengthNonUniformity = nrInv * (range(0, maxRunLength) >> select([rlm](int j) {
                                                 double s = range(0, tons) >> select([j, rlm](int i) {
                                                                return (double)rlm[i][j];
                                                            }) >>
                                                            sum();
                                                 s = pow(s, 2);
                                                 return s;
                                             }) >>
                                             sum());

    double lowGreyLevelRunEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                         return (double)rlm[i][k] / pow((double)i + 1, 2);
                                     });

    double highGreyLevelRunEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                          return (double)rlm[i][k] * pow((double)i + 1, 2);
                                      });

    double shortRunLowGreyLevelEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                              return (double)rlm[i][k] / (pow((double)i + 1, 2) * pow((double)k + 1, 2));
                                          });

    double shortRunHighGreyLevelEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                               return ((double)rlm[i][k] * pow((double)i + 1, 2)) / (pow((double)k + 1, 2));
                                           });

    double longRunLowGreyLevelEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                             return (double)rlm[i][k] / (pow((double)k + 1, 2) * pow((double)i + 1, 2));
                                         });

    double longRunHighGreyLevelEmphasis = nrInv * Sum(tons, maxRunLength, [rlm](int i, int k) {
                                              return (double)rlm[i][k] * pow((double)i + 1, 2) * (pow((double)k + 1, 2));
                                          });

    double numberOfPixels = cropSize * cropSize;

    return MCCStatistics{
#ifdef DEBUG_VIEW
        .visualization = img2,
#endif
        .runPercentage = numberOfRuns / numberOfPixels,
        .shortRunEmphasis = shortRunEmphasis,
        .longRunEmphasis = longRunEmphasis,
        .greyLevelNonUniformity = greyLevelNonUniformity,
        .runLengthNonUniformity = runLengthNonUniformity,
        .lowGreyLevelRunEmphasis = lowGreyLevelRunEmphasis,
        .highGreyLevelRunEmphasis = highGreyLevelRunEmphasis,
        .shortRunLowGreyLevelEmphasis = shortRunLowGreyLevelEmphasis,
        .shortRunHighGreyLevelEmphasis = shortRunHighGreyLevelEmphasis,
        .longRunLowGreyLevelEmphasis = longRunLowGreyLevelEmphasis,
        .longRunHighGreyLevelEmphasis = longRunHighGreyLevelEmphasis};
}

enum Type
{
    Dentina,
    Canal,
    Pino,
    Fundo
};

struct Part
{
    int quantity;
    Type name;

    bool operator<(const Part &d) const
    {
        return d.quantity < quantity;
    }

    bool operator>(const Part &d) const
    {
        return d.quantity > quantity;
    }
};

struct Statistics
{
#ifdef DEBUG_VIEW
    ImageClass croppedImage;
#endif
    Type type;
    int posX;
    int posY;
    std::vector<MCOStatistics> mcoStats;
    MCCStatistics mccStas;
};

std::vector<Statistics> CropImage(std::string image,
                                  std::string truth)
{
    ImageClass img;
    img.Load(image.c_str());

    ImageClass gt;
    gt.Load(truth.c_str());

    int cropped[cropSize][cropSize];

    std::vector<Statistics> results;
    int ignored = 0;

    ImageClass reducedColors;
    reducedColors.SetSize(img.SizeX(), img.SizeY(), 3);
    double tons_mult = 255.0f / (double)tons;

    for (int i = 0; i < img.SizeX(); i ++)
    {
        for (int j = 0; j < img.SizeY(); j ++)
        {
            auto intensity = img.GetPointIntensityRounded(i, j);
            intensity = (int)(intensity / tons_mult);
            reducedColors.SetPointIntensity(i, j, intensity);
        }
    }


    for (int i = 0; i < img.SizeX(); i += cropSize)
    {
        for (int j = 0; j < img.SizeY(); j += cropSize)
        {

            Statistics statistics;


            int dentina = 0;
            int canal = 0;
            int fundo = 0;
            int pino = 0;

            int nonBlackPixels = 0;

            for (int x = 0; x < cropSize; x++)
            {
                for (int y = 0; y < cropSize; y++)
                {
                    int _x = i + x;
                    int _y = j + y;

                    auto intensity = reducedColors.GetPointIntensityRounded(_x, _y);

                    if (intensity != 0)
                    {
                        nonBlackPixels++;
                    }

                    cropped[x][y] = intensity;
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

            if (nonBlackPixels == 0)
            {
                //se for 100% fundo: ignora
                ignored++;
                continue;
            }

            //std::cout << "Cropped " << num << " " << i << " " << j << std::endl;

            std::priority_queue<Part, std::vector<Part>, std::greater<Part>> parts;

            parts.push(Part{.quantity = dentina, .name = Type::Dentina});
            parts.push(Part{.quantity = canal, .name = Type::Canal});
            parts.push(Part{.quantity = pino, .name = Type::Pino});
            parts.push(Part{.quantity = fundo, .name = Type::Fundo});

            auto largest = parts.top();
            statistics.type = largest.name;
            statistics.posX = i;
            statistics.posY = j;
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 1, 0));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 0, 1));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 1, 1));

            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 3, 0));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 0, 3));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 3, 3));

            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 5, 0));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 0, 5));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 5, 5));

            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 15, 0));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 0, 15));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 15, 15));

            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 25, 0));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 0, 25));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 25, 25));

            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 35, 0));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 0, 35));
            statistics.mcoStats.push_back(MCO<tons>(reducedColors, i, j, 35, 35));

            statistics.mccStas = MCC<tons, cropSize, cropSize>(cropped);
#ifdef DEBUG_VIEW
            statistics.croppedImage = cropped;
#endif
            results.push_back(statistics);

#ifdef DEBUG_VIEW
            std::string cropFolder = folder + "/" + std::to_string(num);
            mkdir(cropFolder.c_str(), 0777);
            std::string result = cropFolder + "/crop_" + std::to_string(i) + "_" + std::to_string(j) + ".bmp";
            cropped.Save(result.c_str());
            statistics.mcoStats[4].visualization.Save((cropFolder + "/crop_" + std::to_string(i) + "_" + std::to_string(j) + "_mco.bmp").c_str());
            statistics.mccStas.visualization.Save((cropFolder + "/crop_" + std::to_string(i) + "_" + std::to_string(j) + "_mcc.bmp").c_str());
#endif
        }
    }
    std::cout << "ignored: " << ignored << std::endl;
    gt.Delete();
    img.Delete();
    return results;
}

void writeHeader(ofstream &ofstream)
{
    ofstream << "id,";

    for (int i = 1; i <= 18; i++)
    {
        ofstream << "energy_" << i << ",";
        ofstream << "entropy_" << i << ",";
        ofstream << "contrast_" << i << ",";
        ofstream << "variance_" << i << ",";
        ofstream << "homogeinity_" << i << ",";
    }

    ofstream << "runPercentage,";
    ofstream << "shortRunEmphasis,";
    ofstream << "longRunEmphasis,";
    ofstream << "greyLevelNonUniformity,";
    ofstream << "runLengthNonUniformity,";
    ofstream << "lowGreyLevelRunEmphasis,";
    ofstream << "highGreyLevelRunEmphasis,";
    ofstream << "shortRunLowGreyLevelEmphasis,";
    ofstream << "shortRunHighGreyLevelEmphasis,";
    ofstream << "longRunLowGreyLevelEmphasis,";
    ofstream << "longRunHighGreyLevelEmphasis,class";
    ofstream << std::endl;
}

void writeLine(std::string originalFile, ofstream &ofstream, Statistics &stats)
{

    //print id
    ofstream << "crop" << originalFile << "_pos_x" << stats.posX << "_y" << stats.posY << ",";

    for (auto mcoStatus : stats.mcoStats)
    {
        ofstream << mcoStatus.energy
                 << "," << mcoStatus.entropy
                 << "," << mcoStatus.contrast
                 << "," << mcoStatus.variance
                 << "," << mcoStatus.homogeinity
                 << ",";
    }

    ofstream << stats.mccStas.runPercentage << ",";
    ofstream << stats.mccStas.shortRunEmphasis << ",";
    ofstream << stats.mccStas.longRunEmphasis << ",";
    ofstream << stats.mccStas.greyLevelNonUniformity << ",";
    ofstream << stats.mccStas.runLengthNonUniformity << ",";
    ofstream << stats.mccStas.lowGreyLevelRunEmphasis << ",";
    ofstream << stats.mccStas.highGreyLevelRunEmphasis << ",";
    ofstream << stats.mccStas.shortRunLowGreyLevelEmphasis << ",";
    ofstream << stats.mccStas.shortRunHighGreyLevelEmphasis << ",";
    ofstream << stats.mccStas.longRunLowGreyLevelEmphasis << ",";
    ofstream << stats.mccStas.longRunHighGreyLevelEmphasis << ",";

    switch (stats.type)
    {
    case Type::Canal:
        ofstream << "1";
        break;
    case Type::Dentina:
        ofstream << "2";
        break;
    case Type::Pino:
        ofstream << "3";
        break;
    case Type::Fundo:
        ofstream << "4";
        break;
    default:
        break;
    }
    ofstream << std::endl;
};

std::vector<std::string> listDir(std::string dirPath)
{
    std::vector<std::string> vec;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirPath.c_str())) != NULL)
    {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL)
        {
            std::string fullpath = dirPath + "/" + ent->d_name;
            vec.push_back(fullpath);
        }
        closedir(dir);
    }
    return vec;
}

bool fileExists(const std::string &name)
{
    if (FILE *file = fopen(name.c_str(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}

std::vector<std::string> split(std::string s, std::string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

void tryRenderResult()
{

    ifstream file;
    file.open("previsoes.csv");

    std::string str;
    bool header = true;

    std::map<std::string, std::vector<std::tuple<int, int, Type>>> fileAndResults;

    while (std::getline(file, str))
    {
        if (header)
        {
            header = false;
            continue;
        }

        auto cols = split(str, ",");

        auto x = split(cols[0], "_pos_");
        //crop./images/ImagensOriginais/Dentes 01 02 03_rec0647.png_pos_x1440_y940,4

        auto file = x[0].substr(4, x[0].size() - 4); //tira o crop
        auto posxy = split(x[1], "_");

        auto posx = std::atoi(posxy[0].substr(1, posxy[0].size() - 1).c_str());
        auto posy = std::atoi(posxy[1].substr(1, posxy[1].size() - 1).c_str());

        Type type;

        switch (std::atoi(cols[1].c_str()))
        {
        case 1:
            type = Type::Canal;
            break;
        case 2:
            type = Type::Dentina;
            break;
        case 3:
            type = Type::Pino;
            break;
        case 4:
            type = Type::Fundo;
            break;
        default:
            break;
        }
        if (fileAndResults.find(file) == fileAndResults.end())
        {
            fileAndResults[file] = std::vector<std::tuple<int, int, Type>>();
        }
        fileAndResults[file].push_back(std::make_tuple(posx, posy, type));
        //std::cout << file << ", " << posx << ", " << posy << ", " << type << std::endl;
    }

    for (auto const &element : fileAndResults)
    {
        auto filename = element.first;

        ImageClass img;
        img.Load(filename.c_str());

        ImageClass img2;
        img2.SetSize(img.SizeX(), img.SizeY(), 3);

    //converte img para 3 canais
        for (int i = 0; i < img.SizeX(); i++)
        {
            for (int j = 0; j < img.SizeY(); j++)
            {
                img2.SetPointIntensity(i, j, img.GetPointIntensityRounded(i, j));
            }
        }

        for (auto record : element.second)
        {
            auto x = std::get<0>(record);
            auto y = std::get<1>(record);
            Type t = std::get<2>(record);

            for (int i = x; i < x + cropSize; i++)
            {
                for (int j = y; j < y + cropSize; j++)
                {

                    if (t == Type::Dentina)
                    {
                        img2.DrawPixel(i, j, 0, 255, 0);
                    }
                    else if (t == Type::Canal)
                    {
                        img2.DrawPixel(i, j, 255, 0, 0);
                    }
                }
            }
        }

        auto filenameDrawn = filename + "_result.bmp";

        img2.Save(filenameDrawn.c_str());
        img.Delete();
        img2.Delete();
    }
}

// **********************************************************************
//  void main ( int argc, char** argv )
// **********************************************************************
int main(int argc, char **argv)
{
    //descomentar essas 2 linhas para gerar visualizaçao após treino do classificador.py
    //tryRenderResult();
    //return 0;

    ofstream train;
    train.open("treino.csv");
    writeHeader(train);

    ofstream test;
    test.open("teste.csv");
    writeHeader(test);

    int dentina = 0;
    int canal = 0;
    int fundo = 0;
    int pino = 0;

    auto groundTruthFiles = listDir("./images/GroundTruth");

    for (auto groundTruth : groundTruthFiles)
    {
        //pega o nome do arquivo:

        std::string filenameOnly = basename(groundTruth.c_str()); // = file.find_last_of(".");
        if (filenameOnly == ".." || filenameOnly == ".")
        {
            continue;
        }
        std::string noExtension = filenameOnly.substr(0, filenameOnly.find_last_of("."));

        //tenta abrir bmp, depois png
        std::string pngOriginal = "./images/ImagensOriginais/" + noExtension + ".png";

        if (fileExists(pngOriginal))
        {
            std::cout << "File " << pngOriginal << " exists!" << std::endl;
        }
        else
        {
            std::cout << "File " << pngOriginal << " DOES NOT exists!" << std::endl;
            continue;
        }

        auto cropsAndStats = CropImage(pngOriginal, groundTruth);

        std::cout << "Computed " << groundTruth << " crops: " << cropsAndStats.size() << std::endl;
        for (auto stats : cropsAndStats)
        {
            if (stats.type == Type::Canal)
            {
                canal++;
            }
            if (stats.type == Type::Dentina)
            {
                dentina++;
            }
            if (stats.type == Type::Fundo)
            {
                fundo++;
            }
            if (stats.type == Type::Pino)
            {
                pino++;
            }

            ofstream &stream = drand48() > 0.8 ? test : train;
            writeLine(pngOriginal, stream, stats);
        }
    }

    std::cout << "Dentina = " << dentina << std::endl;
    std::cout << "Canal = " << canal << std::endl;
    std::cout << "Pino = " << pino << std::endl;
    std::cout << "Fundo = " << fundo << std::endl;

    /*
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(arrow_keys);
    glutMouseFunc(Mouse);

    glutIdleFunc(display);

    glutMainLoop(); */
}
