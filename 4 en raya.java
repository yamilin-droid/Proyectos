import java.util.Scanner;

public class CuatroEnRaya {
    private static final int ROWS = 6;
    private static final int COLS = 7;
    private static final char EMPTY = '.';
    private static final Scanner scanner = new Scanner(System.in);
    private static char[][] tablero;
    private static int turno = 0;
    private static int movimientos = 0;
    private static final char[] fichas = {'X', 'O'};

    public static void main(String[] args) {
        tablero = crearTablero();
        siguienteTurno();
    }

    private static char[][] crearTablero() {
        char[][] nuevoTablero = new char[ROWS][COLS];
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                nuevoTablero[i][j] = EMPTY;
            }
        }
        return nuevoTablero;
    }

    private static void imprimirTablero() {
        System.out.print("\033[H\033[2J"); // Limpiar consola
        System.out.flush();
        
        for (char[] fila : tablero) {
            for (char celda : fila) {
                System.out.print(celda + " ");
            }
            System.out.println();
        }
        System.out.println("0 1 2 3 4 5 6");
    }

    private static int colocarFicha(int col, char ficha) {
        for (int fila = ROWS - 1; fila >= 0; fila--) {
            if (tablero[fila][col] == EMPTY) {
                tablero[fila][col] = ficha;
                return fila;
            }
        }
        return -1; // Columna llena
    }

    private static boolean hayGanador(char ficha) {
        // Horizontal
        for (int fila = 0; fila < ROWS; fila++) {
            for (int col = 0; col <= COLS - 4; col++) {
                if (tablero[fila][col] == ficha &&
                    tablero[fila][col + 1] == ficha &&
                    tablero[fila][col + 2] == ficha &&
                    tablero[fila][col + 3] == ficha) {
                    return true;
                }
            }
        }
        
        // Vertical
        for (int fila = 0; fila <= ROWS - 4; fila++) {
            for (int col = 0; col < COLS; col++) {
                if (tablero[fila][col] == ficha &&
                    tablero[fila + 1][col] == ficha &&
                    tablero[fila + 2][col] == ficha &&
                    tablero[fila + 3][col] == ficha) {
                    return true;
                }
            }
        }
        
        // Diagonal \
        for (int fila = 0; fila <= ROWS - 4; fila++) {
            for (int col = 0; col <= COLS - 4; col++) {
                if (tablero[fila][col] == ficha &&
                    tablero[fila + 1][col + 1] == ficha &&
                    tablero[fila + 2][col + 2] == ficha &&
                    tablero[fila + 3][col + 3] == ficha) {
                    return true;
                }
            }
        }
        
        // Diagonal /
        for (int fila = 3; fila < ROWS; fila++) {
            for (int col = 0; col <= COLS - 4; col++) {
                if (tablero[fila][col] == ficha &&
                    tablero[fila - 1][col + 1] == ficha &&
                    tablero[fila - 2][col + 2] == ficha &&
                    tablero[fila - 3][col + 3] == ficha) {
                    return true;
                }
            }
        }
        
        return false;
    }

    private static void siguienteTurno() {
        imprimirTablero();
        int jugador = turno % 2;
        System.out.print("Jugador " + fichas[jugador] + " (X/O), elige columna (0-6): ");
        
        try {
            int col = scanner.nextInt();
            if (col < 0 || col > 6) {
                System.out.println("Columna inválida. Intenta de nuevo.");
                siguienteTurno();
                return;
            }

            int fila = colocarFicha(col, fichas[jugador]);
            if (fila == -1) {
                System.out.println("Columna llena. Elige otra.");
                siguienteTurno();
                return;
            }

            movimientos++;
            if (hayGanador(fichas[jugador])) {
                imprimirTablero();
                System.out.println("¡Jugador " + fichas[jugador] + " gana!");
                scanner.close();
                return;
            }

            if (movimientos == ROWS * COLS) {
                imprimirTablero();
                System.out.println("¡Empate!");
                scanner.close();
                return;
            }

            turno++;
            siguienteTurno();
        } catch (Exception e) {
            System.out.println("Entrada inválida. Intenta de nuevo.");
            scanner.nextLine(); // Limpiar el buffer
            siguienteTurno();
        }
    }
}