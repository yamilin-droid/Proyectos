
using System;

class TicTacToe
{
    static char[] board = { '1','2','3','4','5','6','7','8','9' };
    static char currentPlayer = 'X';

    static void Main()
    {
        int moves = 0;
        bool gameWon = false;

        do
        {
            Console.Clear();
            PrintBoard();
            Console.WriteLine($"Turno del jugador {currentPlayer}. Elige una casilla (1-9):");
            string input = Console.ReadLine();

            if (int.TryParse(input, out int pos) && pos >= 1 && pos <= 9 && board[pos - 1] != 'X' && board[pos - 1] != 'O')
            {
                board[pos - 1] = currentPlayer;
                moves++;
                gameWon = CheckWin();

                if (!gameWon)
                    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
            }
            else
            {
                Console.WriteLine("Movimiento invalido. Presiona Enter para continuar.");
                Console.ReadLine();
            }
        }
        while (!gameWon && moves < 9);

        Console.Clear();
        PrintBoard();
        if (gameWon)
            Console.WriteLine($"¡El jugador {currentPlayer} ha ganado!");
        else
            Console.WriteLine("¡Empate!");
    }

    static void PrintBoard()
    {
        Console.WriteLine("-------------");
        for (int i = 0; i < 9; i += 3)
        {
            Console.WriteLine($"| {board[i]} | {board[i+1]} | {board[i+2]} |");
            Console.WriteLine("-------------");
        }
    }

    static bool CheckWin()
    {
        int[,] winPositions = {
            {0,1,2}, {3,4,5}, {6,7,8}, 
            {0,3,6}, {1,4,7}, {2,5,8}, 
            {0,4,8}, {2,4,6}           
        };

        for (int i = 0; i < winPositions.GetLength(0); i++)
        {
            if (board[winPositions[i,0]] == currentPlayer &&
                board[winPositions[i,1]] == currentPlayer &&
                board[winPositions[i,2]] == currentPlayer)
                return true;
        }
        return false;
    }
}