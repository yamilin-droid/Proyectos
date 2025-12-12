#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

class Nodo {
public:
    int clave;
    Nodo* izquierda;
    Nodo* derecha;

    Nodo(int k) : clave(k), izquierda(nullptr), derecha(nullptr) {}
};

class ABB {
private:
    Nodo* raiz;
    int numNodos;

    Nodo* insertarNodo(Nodo* nodo, int clave) {
        if (!nodo) {
            numNodos++;
            return new Nodo(clave);
        }
        if (clave < nodo->clave) {
            nodo->izquierda = insertarNodo(nodo->izquierda, clave);
        } else if (clave > nodo->clave) {
            nodo->derecha = insertarNodo(nodo->derecha, clave);
        }
        return nodo;
    }

    Nodo* buscarNodo(Nodo* nodo, int clave, std::string& ruta) const {
        if (!nodo) return nullptr;
        if (clave == nodo->clave) return nodo;
        else if (clave < nodo->clave) {
            ruta += "I ";
            return buscarNodo(nodo->izquierda, clave, ruta);
        } else {
            ruta += "D ";
            return buscarNodo(nodo->derecha, clave, ruta);
        }
    }

    Nodo* eliminarNodo(Nodo* nodo, int clave) {
        if (!nodo) return nullptr;
        if (clave < nodo->clave) {
            nodo->izquierda = eliminarNodo(nodo->izquierda, clave);
        } else if (clave > nodo->clave) {
            nodo->derecha = eliminarNodo(nodo->derecha, clave);
        } else {
            if (!nodo->izquierda) {
                Nodo* temp = nodo->derecha;
                delete nodo;
                numNodos--;
                return temp;
            } else if (!nodo->derecha) {
                Nodo* temp = nodo->izquierda;
                delete nodo;
                numNodos--;
                return temp;
            }
            Nodo* temp = encontrarMinimo(nodo->derecha);
            nodo->clave = temp->clave;
            nodo->derecha = eliminarNodo(nodo->derecha, temp->clave);
        }
        return nodo;
    }

    Nodo* encontrarMinimo(Nodo* nodo) const {
        while (nodo && nodo->izquierda) nodo = nodo->izquierda;
        return nodo;
    }

    void inordenHelper(Nodo* nodo, std::vector<int>& resultado) const {
        if (!nodo) return;
        inordenHelper(nodo->izquierda, resultado);
        resultado.push_back(nodo->clave);
        inordenHelper(nodo->derecha, resultado);
    }

    void preordenHelper(Nodo* nodo, std::vector<int>& resultado) const {
        if (!nodo) return;
        resultado.push_back(nodo->clave);
        preordenHelper(nodo->izquierda, resultado);
        preordenHelper(nodo->derecha, resultado);
    }

    void postordenHelper(Nodo* nodo, std::vector<int>& resultado) const {
        if (!nodo) return;
        postordenHelper(nodo->izquierda, resultado);
        postordenHelper(nodo->derecha, resultado);
        resultado.push_back(nodo->clave);
    }

    int alturaHelper(Nodo* nodo) const {
        if (!nodo) return 0;
        return 1 + std::max(alturaHelper(nodo->izquierda), alturaHelper(nodo->derecha));
    }

    void destruirArbol(Nodo* nodo) {
        if (nodo) {
            destruirArbol(nodo->izquierda);
            destruirArbol(nodo->derecha);
            delete nodo;
        }
    }

public:
    ABB() : raiz(nullptr), numNodos(0) {}

    ~ABB() {
        destruirArbol(raiz);
    }

    void insertar(int clave) {
        raiz = insertarNodo(raiz, clave);
    }

    std::string buscar(int clave) const {
        std::string ruta;
        Nodo* resultado = buscarNodo(raiz, clave, ruta);
        return resultado ? ruta : "No encontrado";
    }

    void eliminar(int clave) {
        raiz = eliminarNodo(raiz, clave);
    }

    std::vector<int> inorden() const {
        std::vector<int> resultado;
        inordenHelper(raiz, resultado);
        return resultado;
    }

    std::vector<int> preorden() const {
        std::vector<int> resultado;
        preordenHelper(raiz, resultado);
        return resultado;
    }

    std::vector<int> postorden() const {
        std::vector<int> resultado;
        postordenHelper(raiz, resultado);
        return resultado;
    }

    int altura() const {
        return alturaHelper(raiz);
    }

    int tamano() const {
        return numNodos;
    }

    bool estaVacio() const {
        return raiz == nullptr;
    }

    bool exportarInorden(const std::string& nombreArchivo) const {
        std::ofstream archivo(nombreArchivo);
        if (!archivo) return false;

        auto in = inorden();
        for (size_t i = 0; i < in.size(); ++i) {
            archivo << in[i];
            if (i != in.size() - 1) archivo << " ";
        }
        return true;
    }
};

void mostrarVector(const std::vector<int>& vec) {
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i];
        if (i != vec.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
}

void mostrarAyuda() {
    std::cout << "\nComandos disponibles:\n"
              << "  insertar <numero>  - Insertar un numero en el arbol\n"
              << "  buscar <numero>    - Buscar un numero y mostrar la ruta\n"
              << "  eliminar <numero>  - Eliminar un numero del arbol\n"
              << "  inorden            - Mostrar recorrido inorden\n"
              << "  preorden           - Mostrar recorrido preorden\n"
              << "  postorden          - Mostrar recorrido postorden\n"
              << "  altura             - Mostrar altura del arbol\n"
              << "  tamano             - Mostrar numero de nodos\n"
              << "  exportar <archivo> - Exportar recorrido inorden a archivo\n"
              << "  ayuda              - Mostrar esta ayuda\n"
              << "  salir              - Salir del programa\n" << std::endl;
}

int main() {
    ABB arbol;
    std::string comando;

    std::cout << "Implementacion de Arbol Binario de Busqueda (ABB)" << std::endl;
    std::cout << "Escriba 'ayuda' para ver los comandos disponibles" << std::endl;

    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, comando);

        std::istringstream iss(comando);
        std::string cmd;
        iss >> cmd;

        if (cmd == "insertar") {
            int num;
            if (iss >> num) {
                arbol.insertar(num);
                std::cout << "Se inserto " << num << std::endl;
            } else {
                std::cout << "Error: Por favor proporcione un numero valido" << std::endl;
            }
        }
        else if (cmd == "buscar") {
            int num;
            if (iss >> num) {
                std::string ruta = arbol.buscar(num);
                if (ruta != "No encontrado") {
                    std::cout << "Encontrado " << num << " (Ruta: " << ruta << ")" << std::endl;
                } else {
                    std::cout << num << " no se encontro en el arbol" << std::endl;
                }
            } else {
                std::cout << "Error: Por favor proporcione un numero a buscar" << std::endl;
            }
        }
        else if (cmd == "eliminar") {
            int num;
            if (iss >> num) {
                std::string ruta = arbol.buscar(num);
                if (ruta != "No encontrado") {
                    arbol.eliminar(num);
                    std::cout << "Se elimino " << num << std::endl;
                } else {
                    std::cout << num << " no se encontro en el arbol" << std::endl;
                }
            } else {
                std::cout << "Error: Por favor proporcione un numero a eliminar" << std::endl;
            }
        }
        else if (cmd == "inorden") {
            std::vector<int> resultado = arbol.inorden();
            std::cout << "Recorrido Inorden: ";
            mostrarVector(resultado);
        }
        else if (cmd == "preorden") {
            std::vector<int> resultado = arbol.preorden();
            std::cout << "Recorrido Preorden: ";
            mostrarVector(resultado);
        }
        else if (cmd == "postorden") {
            std::vector<int> resultado = arbol.postorden();
            std::cout << "Recorrido Postorden: ";
            mostrarVector(resultado);
        }
        else if (cmd == "altura") {
            std::cout << "Altura del arbol: " << arbol.altura() << std::endl;
        }
        else if (cmd == "tamano") {
            std::cout << "Numero de nodos: " << arbol.tamano() << std::endl;
        }
        else if (cmd == "exportar") {
            std::string nombreArchivo;
            if (iss >> nombreArchivo) {
                if (arbol.exportarInorden(nombreArchivo)) {
                    std::cout << "Recorrido inorden exportado a " << nombreArchivo << std::endl;
                } else {
                    std::cout << "Error: No se pudo escribir en el archivo" << std::endl;
                }
            } else {
                std::cout << "Error: Por favor proporcione un nombre de archivo" << std::endl;
            }
        }
        else if (cmd == "ayuda") {
            mostrarAyuda();
        }
        else if (cmd == "salir") {
            std::cout << "Saliendo..." << std::endl;
            break;
        }
        else {
            std::cout << "Comando desconocido. Escriba 'ayuda' para ver los comandos disponibles." << std::endl;
        }
    }

    return 0;
}
