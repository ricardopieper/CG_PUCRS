#include <stdio.h>
#include <math.h>
#include <iostream>

class Vetor{
    double x,y;
public:
    Vetor(){ x=0; y=0;} // construtora
    Vetor(double _x, double _y){ x=_x; y=_y;} // construtora
    void set(double x, double y)
    {
        this->x = x;
        this->y = y;
    }
    void get (double &x, double &y)
    {
        x = this->x;
        y = this->y;
    }
    double getX() { return this->x; }
    double getY() { return this->y; }
    void add(Vetor v)
    {
        this->x += v.getX();
        this->y += v.getY();
    }
    void multiply(double n) { this->x *=n; this->y *=n;}
    void set(Vetor Inicial, Vetor Final)
    {
        this->x = Final.getX()-Inicial.getX();
        this->y = Final.getY()-Inicial.getY();
    }
    void imprime()
    {
        printf("%f, %f", x,y);
    }
    double modulo()
    {
        double m;
        m = sqrt(x*x+y*y);
        return m;
    }
    Vetor getVersor()
    {
        return Vetor(x/modulo(), y/modulo());
    }
    
};

double ProdutoEscalar(Vetor V1, Vetor V2)
{
    double Escalar;
    Escalar = V1.getX()* V2.getX() + V1.getY()* V2.getY();
    return Escalar;
}

double ProdutoVetorial(Vetor V1, Vetor V2)
{
    double Vetorial;
    Vetorial = V1.getX()* V2.getY() - V1.getY()* V2.getX();
    return Vetorial;
}

int ClassificaPoligono(int num_vert, Vetor pontos[])
{
    int classe = 1;
    // a rotina deve retornar 1 quando o poligono
    // for convexo e 0 quando for concavo
    
    for (int i = 0; i< num_vert - 2; i++) {

        //eu to no pontos[0]
        Vetor myPosition = pontos[i];
        Vetor lookingAt = pontos[i+1];
        Vetor target = pontos[i+2];

        Vetor a = Vetor(lookingAt.getX() - myPosition.getX(), lookingAt.getY() - myPosition.getY())
            .getVersor(); 
        Vetor b = Vetor(target.getX() - myPosition.getX(), target.getY() - myPosition.getY())
            .getVersor(); 


        double produtoVetorial = ProdutoVetorial(a, b);

        //std::cout<<"produto vetorial vetores "<<i+1<<", "<<i+2<<", "<<i+3<<" = "<<produtoVetorial<<std::endl;

        if (produtoVetorial > 0) {
            classe = 0;
            break;
        }
    }


    return classe;
}

bool intersec2d(Vetor k, Vetor l, Vetor m, Vetor n, double &s, double &t) {

    double det = (n.getX()-m.getX()) * (l.getY() -k.getY())-(n.getY() -m.getY()) * (l.getX()-k.getX());
    if (det == 0.0) return false ;  //não há intersecç
    s = ((n.getX()-m.getX()) * (m.getY() -k.getY()) -(n.getY() -m.getY()) * (m.getX()-k.getX()))/ det ;
    t = ((l.getX()-k.getX()) * (m.getY() -k.getY()) -(l.getY() -k.getY()) * (m.getX()-k.getX()))/ det ;
    return true;  // há intersecç
}


bool minOrMaxLocal(int num_vert, Vetor pontos[], int index) {

    int vizinho1 = index - 1;
    int vizinho2 = index + 1;

    if (index == 0) {
        vizinho1 = num_vert - 1;
    }

    if (index == num_vert - 1) {
        vizinho2 = 0;
    }

    Vetor a = pontos[vizinho1];
    Vetor b = pontos[vizinho2];
    //local max
    if (pontos[index].getY() >= a.getY() && pontos[index].getY() >= b.getY()) {
        return true;
    }

    //local min
    if (pontos[index].getY() <= a.getY() && pontos[index].getY() <= b.getY()) {
        return true;
    }

    return false;
}

int EstaDentroConcavo(int num_vert, Vetor pontos[], Vetor ponto)
{
    // a rotina deve retornar 1 quando o ponto
    // estiver dentro do poli­gono e 0 se estiver fora
    
    Vetor pontoInicio = Vetor(0, ponto.getY());
    Vetor pontoFim = ponto;

    int intersecoes = 0;

    for (int i = 0; i< num_vert - 1; i++) {
        double s = -1, t = -1;
        if (intersec2d(pontos[i], pontos[i+1], pontoInicio, pontoFim, s, t)) {
            intersecoes++;
            if (minOrMaxLocal(num_vert, pontos, i)) {
                intersecoes++;
            }
        }
    }


    return intersecoes % 2;

}
int EstaDentroConvexo(int num_vert, Vetor pontos[], Vetor ponto)
{
    Vetor pontoInicio = Vetor(0, ponto.getY());
    Vetor pontoFim = ponto;

    int classe;
    // a rotina deve retornar 1 quando o ponto
    // estiver dentro do poli­gono e 0 se estiver fora
    
    int ultimoLado = -1;

    for (int i = 0; i< num_vert - 1; i++) {
        Vetor myPosition = pontos[i];
        Vetor lookingAt = pontos[i+1];
        Vetor target = ponto;

        Vetor a = Vetor(lookingAt.getX() - myPosition.getX(), lookingAt.getY() - myPosition.getY())
            .getVersor(); 
        Vetor b = Vetor(target.getX() - myPosition.getX(), target.getY() - myPosition.getY())
            .getVersor(); 


        double produtoVetorial = ProdutoVetorial(a, b);

        int lado = produtoVetorial > 0;

        if (ultimoLado == -1) {
            ultimoLado = lado;
        }
        else {
            if (lado != ultimoLado) {
                return 0;
            }
            ultimoLado = lado;
        }


    }
    return 1;
}

//tentando fazer o exemplo da pagina
void ClassificaPoligonoConcavoExemplo() {

    Vetor V1 = Vetor(0, 0);
    Vetor V2 = Vetor(0, 50);
    Vetor V3 = Vetor(50, 50);
    Vetor V4 = Vetor(50, 10);
    Vetor V5 = Vetor(40, 30);//concavidade
    Vetor V6 = Vetor(30, 0);
    
    Vetor vetores[] = {V1,V2,V3,V4,V5,V6};

    int classe = ClassificaPoligono(6, vetores);

    std::cout << "Teste Poligono classe concavo: "<<(classe == 0? "PASSOU" : "FALHOU") << std::endl;

}


//tentando fazer o exemplo da pagina
void ClassificaPoligonoConvexoExemplo() {

    Vetor V1 = Vetor(0, 0);
    Vetor V2 = Vetor(0, 50);
    Vetor V3 = Vetor(50, 50);
    Vetor V4 = Vetor(50, 10);
    Vetor V5 = Vetor(40, 0); //no outro exemplo esse aqui forma concavidade, aqui é convexo
    Vetor V6 = Vetor(30, 0);
    
    Vetor vetores[] = {V1,V2,V3,V4,V5,V6};

    int classe = ClassificaPoligono(6, vetores);

    std::cout << "Teste Poligono classe convexo: "<<(classe == 1? "PASSOU" : "FALHOU") << std::endl;

}


int main()
{
    Vetor V1, V2;
    Vetor A, B, P1;
    A.set(5,2);
    B.set(10,2);
    P1.set(7.5,2.0);
    
    V1.set(A, B);
    
    V2.set(A, P1);
    
    
    printf("Prod Escalar: %f\n", ProdutoEscalar(V1.getVersor(),V2.getVersor()));
    
    ClassificaPoligonoConcavoExemplo();
    ClassificaPoligonoConvexoExemplo();
    //V1.imprime();
    printf("\n");
    return 0;
}