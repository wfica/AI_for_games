// #define DIAGNOSTICS

using System;
using System.Linq;
using System.IO;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Numerics;
using System.Diagnostics;
using System.Runtime;

#nullable enable

// board:
// 00 01 02
// 10 11 12
// 20 21 22
class Player
{
    static void Main(string[] args)
    {
        // GCSettings.LatencyMode = GCLatencyMode.SustainedLowLatency;

        // Utils.Duels(3); // used to compare two agents locally
        // return;

        var agent = new MCTS<ComplexBoard>(new ComplexBoard());
        // var agent = new FlatMC<ComplexBoard>(new ComplexBoard());

        // Utils.SimulationTimes(agent, 5); // used to estimate turn times
        // return;

        // game loop
        while (true)
        {
            var gcb = GC.TryStartNoGCRegion(244_000_000);
            // Runtime.Assert(gcb, "TryStartNoGCRegion");

            var inputs = Console.ReadLine()!.Split(' ');
            int opponentRow = int.Parse(inputs[0]);
            int opponentCol = int.Parse(inputs[1]);
            agent.OpponentMove(opponentRow, opponentCol);

            int validActionCount = int.Parse(Console.ReadLine()!);
            for (int i = 0; i < validActionCount; i++)
            {
                inputs = Console.ReadLine()!.Split(' ');
                int row = int.Parse(inputs[0]);
                int col = int.Parse(inputs[1]);
                // // Check that agent's board suggests allowed moves
                // int move = agent.root.board.RowColToMove(row, col);
                // int prev = move - 1;
                // // Console.Error.WriteLine($"Debug: {row},{col} vs {move}");
                // Runtime.Assert(agent.root.board.NextMove(ref prev) && prev == move, $"agent.board.NextMove(ref {prev}) && {prev} == {move}");
            }
            // // Check that agent's board suggests only allowed moves
            // int m = -1;
            // int c = 0;
            // while (agent.root.board.NextMove(ref m))
            //     c++;
            // // if (c != validActionCount)
            // //     Console.Error.WriteLine($"lastMove={agent.root.board.nextBoard}; agent.root.board.isTerminal={agent.root.board.isTerminal}");
            // Runtime.Assert(c == validActionCount, $"c={c} == validActionCount={validActionCount}");

            // Write an action using Console.WriteLine()
            // To debug: Console.Error.WriteLine("Debug messages...");

            agent.MyMove(out var myRow, out var myCol);
            Console.WriteLine($"{myRow} {myCol}");

            GC.EndNoGCRegion();
            GC.Collect();
        }
    }
}

internal interface IAgent<T>
    where T : IBoard<T>
{
    void Init();
    void MyMove(out int myRow, out int myCol);
    void OpponentMove(int row, int col);
    T Board { get; }
}

internal class FlatMC<T> : IAgent<T>
    where T : IBoard<T>
{
    internal T board = default!;

    public FlatMC(T board)
    {
        this.board = board;
    }

    public T Board => board;

    public void Init() { }

    public void MyMove(out int myRow, out int myCol)
    {
        var move = -1;

        var bestMove = move;
        var bestScore = int.MinValue;
        // Runtime.Assert(!board.IsTerminal(out _), "!board.IsTerminal(out _)");
        while (board.NextMove(ref move))
        {
            if (board.EvaluateMove(move) is var score && score > bestScore)
            {
                bestScore = score;
                bestMove = move;
            }
        }
        board.MoveToRowCol(bestMove, out myRow, out myCol);
        board.Move(XO.X, myRow, myCol);
    }

    public void OpponentMove(int row, int col)
    {
        if (row >= 0 && col >= 0)
            board.Move(XO.O, row, col);
    }
}

internal class MCTS<T> : IAgent<T>
    where T : IBoard<T>
{
    internal Node root = null!;
    private readonly T initBoard;

#if DIAGNOSTICS
    private static long loopsPerMs = 0L;
    private static int loopsPerMsSamples = 0;
#endif

    public MCTS(T board)
    {
        this.initBoard = board;
    }

    public T Board => root != null ? root.board : initBoard;

    public void Init()
    {
        this.root = new Node(initBoard, parent: null, XO.X);

        Grow(Parameters.MCTS_WARMUP_STEPS, Parameters.MCTS_WARMUP_MS);
    }

    public void MyMove(out int myRow, out int myCol)
    {
        if (root is null)
            Init();

        Grow(Parameters.MCTS_STEPS, Parameters.MCTS_MS);

        root!.BestChild(out var index, isFinal: true);
        // Runtime.Assert(index >= 0, "index >= 0");
        var move = root.moves[index];
        root.board.MoveToRowCol(move, out myRow, out myCol);
        // Runtime.Assert(root.xo == XO.X, "root.xo == XO.X");
        root = root.Move(myRow, myCol);
    }

    public void OpponentMove(int row, int col)
    {
        if (row >= 0 && col >= 0)
        {
            if (root is null)
                initBoard.Move(XO.O, row, col);
            else
            {
                // Runtime.Assert(root.xo == XO.O, "root.xo == XO.O");
                root = root.Move(row, col);
            }
        }
    }

    private void Grow(int steps, long ms)
    {
        var sw = new Stopwatch();
        sw.Start();
        var loops = 0;
        do
        {
            Runtime.Assert(GCSettings.LatencyMode == GCLatencyMode.NoGCRegion, "GCSettings.LatencyMode == GCLatencyMode.NoGCRegion");
            do
            {
                var v = root.Select();
                var delta = v.Simulate();
                v.Backprop(delta);
                loops++;
            } while (steps-- > 0);
        } while (sw.ElapsedMilliseconds < ms);
#if DIAGNOSTICS
        loopsPerMs = (loopsPerMs * loopsPerMsSamples + (loops / sw.ElapsedMilliseconds)) / (loopsPerMsSamples + 1);
        loopsPerMsSamples++;
        Console.Error.WriteLine($"{loopsPerMs} ({loops / sw.ElapsedMilliseconds}) loops per 1ms; {loops}");
#endif
    }

    internal class Node
    {
        internal readonly T board;
        internal readonly XO xo; // next move's
        internal readonly ImmutableArray<int> moves;

        private readonly Node[] children;

        private Node? parent;
        private int nextMoveIndex = 0;
        private int visits = 0; // N
        private int score = 0; // Q

        public Node(T board, Node? parent, XO xo)
        {
            this.board = board;
            this.parent = parent;
            this.xo = xo;

            // Generate shuffled available moves
            var builder = ImmutableArray.CreateBuilder<int>();
            var move = -1;
            while (board.NextMove(ref move))
                builder.Add(move);
            Runtime.random.Shuffle(builder);
            this.children = new Node[builder.Count];
            this.moves = builder.ToImmutable();
        }

        public Node Select()
        {
            var v = this;

            // while v is not terminal
            while (v.moves.Length > 0)
            {
                // if v not fully expanded...
                if (v.nextMoveIndex < v.moves.Length)
                    return v.Expand();
                v = v.BestChild(out _);
            }
            return v;
        }

        public Node BestChild(out int index, bool isFinal = false, double c = Parameters.C)
        {
            var v = this;
            // Runtime.Assert(v.nextMoveIndex == v.moves.Length, "v.nextMoveIndex == v.moves.Length");
            // Runtime.Assert(v.moves.Length > 0, "v.moves.Length > 0");
            var nv = v.visits;
            var bestChild = default(Node);
            index = -1;
            var best = double.MinValue;
            for (int i = 0; i < v.children.Length; i++)
            {
                var child = v.children[i];
                int n = child.visits;
                var val = isFinal
                    ? n
                    : ((double)child.score / (double)n) + c * Math.Sqrt(2 * Math.Log(nv) / (double)n);
                if (val > best)
                {
                    best = val;
                    bestChild = child;
                    index = i;
                }
            }
            // Runtime.Assert(bestChild != null, "bestChild != null");
            return bestChild!;
        }

        public Node Expand()
        {
            // Runtime.Assert(nextMoveIndex < moves.Length, "nextMoveIndex < moves.Length");
            var childBoard = board.Copy();
            childBoard.MoveToRowCol(moves[nextMoveIndex], out var row, out var col);
            childBoard.Move(xo, row, col);
            return children[nextMoveIndex++] = new Node(childBoard, this, 4 - xo);
        }

        public int Simulate()
            => board.DefaultPolicy(xo, move: -1);

        public void Backprop(int delta)
        {
            Node? v = this;
            while (v != null)
            {
                v.visits++;
                v.score += delta;
                v = v.parent;
            }
        }

        internal Node Move(int row, int col)
        {
            int move = board.RowColToMove(row, col);
            var i = moves.IndexOf(move);
            // Runtime.Assert(i >= 0, $"i >= 0; {xo}, {row}, {col}, {move}, {moves.Length}");
            // Runtime.Assert(i < nextMoveIndex, "i < nextMoveIndex");
            var child = children[i];
            child.parent = null;
            return child;
        }
    }
}



internal interface IBoard<T>
{
    int EvaluateMove(int move, int iterations = Parameters.SIMULATIONS);
    bool IsTerminal(out Winner winner);
    void Move(XO xo, int row, int col);
    bool NextMove(ref int move);
    void MoveToRowCol(int move, out int row, out int col);
    int RowColToMove(int row, int col);
    T Copy();

    int DefaultPolicy(XO xo, int move);
}

internal class ComplexBoard : IBoard<ComplexBoard>
{
    private readonly Board[] boards = new Board[] {
        default, default, default,
        default, default, default,
        default, default, default,
    };

    private Board main = default;
    private int nextBoard = -1;
    private int isTerminal = 0;
    internal OptionalWinner winner = OptionalWinner.Empty;

    public int EvaluateMove(int move, int iterations = Parameters.SIMULATIONS)
    {
        var score = 0;
        while (iterations-- > 0)
            score += DefaultPolicy(XO.O, move);
        return score;
    }

    public int DefaultPolicy(XO xo, int move)
    {
        // Save();
        for (int i = 0; i < 9; i++)
            boards[i].Save();
        main.Save();
        var savedNextBoard = nextBoard;
        var savedIsTerminal = isTerminal;
        var savedWinner = this.winner;

        int row, col;
        if (move != -1)
        {
            MoveToRowCol(move, out row, out col);
            Move(XO.X, row, col);
        }

        Winner winner;
        while (!IsTerminal(out winner))
        {
            move = RandomMove();
            MoveToRowCol(move, out row, out col);
            Move(xo, row, col);
            xo = 4 - xo;
        }

        // Restore();
        for (int i = 0; i < 9; i++)
            boards[i].Restore();
        main.Restore();
        nextBoard = savedNextBoard;
        isTerminal = savedIsTerminal;
        this.winner = savedWinner;

        return (int)winner - 2;
    }

    public bool IsTerminal(out Winner winner)
    {
        if (main.IsTerminal(out winner))
        {
            // Runtime.Assert((Winner)this.winner == winner, "this.winner == winner");
            return true;
        }
        winner = Winner.Draw;
        // Runtime.Assert(isTerminal != Board.FULL || (Winner)this.winner == winner, "isTerminal != Board.FULL || (Winner)this.winner == winner");
        return isTerminal == Board.FULL;
    }

    public bool NextMove(ref int move)
    {
        if (winner != OptionalWinner.Empty)
            return false; // no more moves - the state is terminal

        // Runtime.Assert(-1 <= move && move < 9 * 9, "-1 <= move && move < 9*9");

        var targetBoardIndex = nextBoard;
        if (targetBoardIndex >= 0)
        {
            // Runtime.Assert(!IsTerminal(targetBoardIndex), "!IsTerminal(targetBoardIndex)");
            int firstBoardMove = targetBoardIndex * 9;
            if (!(firstBoardMove <= move && move < firstBoardMove + 9))
                move = -1;
            int localMove = move % 9;
            if (!boards[targetBoardIndex].NextMove(ref localMove))
            {
                // Console.Error.WriteLine($"lastMove={lastMove}; targetBoardIndex={targetBoardIndex}; move={move}; localMove={localMove}");
                move = -1;
                return false;
            }
            // Console.Error.WriteLine($"lastMove={lastMove}; targetBoardIndex={targetBoardIndex}; move={move}; localMove={localMove}");
            move = targetBoardIndex * 9 + localMove;
            return true;
        }

        int localMove2 = move % 9;
        for (int boardIndex = move / 9; boardIndex < 9; boardIndex++)
        {
            if (IsTerminal(boardIndex))
                localMove2 = -1;
            else if (boards[boardIndex].NextMove(ref localMove2))
            {
                move = boardIndex * 9 + localMove2;
                return true;
            }
        }

        return false;
    }

    public void Move(XO xo, int row, int col)
    {
        // Runtime.Assert(this.winner == OptionalWinner.Empty, "this.winner == OptionalWinner.Empty");
        int outerRow = row / 3;
        int outerCol = col / 3;

        int r = row % 3;
        int c = col % 3;
        int tagetNextBoard = r * 3 + c;

        int boardIndex = outerRow * 3 + outerCol;
        boards[boardIndex].Move(xo, r, c);
        if (boards[boardIndex].IsTerminal(out var winner))
        {
            if ((isTerminal |= 1 << boardIndex) == Board.FULL)
                this.winner = OptionalWinner.Draw;
            if (winner != Winner.Draw)
            {
                main.Move((XO)winner, outerRow, outerCol);
                if (main.IsTerminal(out winner))
                    this.winner = (OptionalWinner)winner;
            }

        }

        nextBoard = IsTerminal(tagetNextBoard) ? -1 : tagetNextBoard;
    }

    private bool IsTerminal(int boardIndex)
        => (isTerminal & (1 << boardIndex)) != 0;

    public int RandomMove()
    {
        var r = 1 + Runtime.random.Next(BitOperations.PopCount((uint)isTerminal ^ Board.FULL)); // 1..numberOfBoardsWithAvailableMoves
        var boardIndex = Utils.IndexOfRthZero(isTerminal, r);

        //// Runtime.Assert(!IsTerminal(out _), "!IsTerminal(out _)");
        ////////var times = 1 + Runtime.random.Next(BitOperations.PopCount((uint)isTerminal ^ Board.FULL)); // 1..max
        ////var times = r;
        ////var boardIndex = -1;
        ////do
        ////{
        ////    if (!IsTerminal(++boardIndex))
        ////        times--;
        ////} while (times > 0);
        ////Runtime.Assert(boardIndex == boardIndex0, "boardIndex == boardIndex0");
        var localMove = boards[boardIndex].RandomMove();
        return localMove + 9 * boardIndex;
    }

    public void MoveToRowCol(int move, out int row, out int col)
    {
        var prevBoards = move / 9;
        var localMove = move % 9;
        var localRow = localMove / 3;
        var localCol = localMove % 3;
        row = localRow + (prevBoards / 3) * 3;
        col = localCol + (prevBoards % 3) * 3;
    }

    public int RowColToMove(int row, int col)
    {
        int outerRow = row / 3;
        int outerCol = col / 3;
        int prevBoards = outerRow * 3 + outerCol;
        return 9 * prevBoards + (row % 3) * 3 + (col % 3);
    }

    public ComplexBoard Copy()
    {
        var copy = new ComplexBoard();
        for (int i = 0; i < 9; i++)
            copy.boards[i] = boards[i];
        copy.main = main;
        copy.nextBoard = nextBoard;
        copy.isTerminal = isTerminal;
        return copy;
    }
}

internal struct Board : IBoard<Board>
{
    public const int FULL = 0b111111111;

    static ImmutableArray<int> rowTerminals = ImmutableArray.Create(
        0b000000111, // row=0
        0b000111000, // row=1
        0b111000000  // row=2 
    );

    static ImmutableArray<int> colTerminals = ImmutableArray.Create(
        0b001001001, // col=0
        0b010010010, // col=1
        0b100100100  // col=2 
    );

    static ImmutableArray<int> diagTerminals = ImmutableArray.Create(
        0b100010001,
        0b001010100
    );

    // a board, e.g.
    //   x x .
    //   o x .
    //   . o x
    // is represented as two numbers, e.g.
    //   xs: 100010011
    //   os: 010001000
    // such that: board[i,j] = 'x' when xs & i = 1
    //                       = 'o' when os & i = 1
    //                       = '.' otherwise
    private int xs;
    private int os;
    private OptionalWinner winner;

    private int xsCache;
    private int osCache;
    private OptionalWinner winnerCache;

    public bool IsTerminal(out Winner winner)
        => (OptionalWinner)(winner = (Winner)this.winner) != OptionalWinner.Empty;

    public int EvaluateMove(int move, int iterations = Parameters.SIMULATIONS) => throw new NotImplementedException();
    public int DefaultPolicy(XO xo, int move) => throw new NotImplementedException();

    public bool NextMove(ref int move)
    {
        // Runtime.Assert(!IsTerminal(out _), "!IsTerminal(out _)");
        // Runtime.Assert(-1 <= move && move < 9, "-1 <= move && move < 9");
        var board = xs | os;
        var i = 1 << (move + 1);
        while (move++ < 9)
        {
            if ((board & i) == 0)
                break;
            i <<= 1;
        }
        if (move == 9)
            move = -1;
        return move != -1;
    }

    public int RandomMove()
    {
        var board = xs | os;
        var r = 1 + Runtime.random.Next(BitOperations.PopCount((uint)board ^ FULL)); // 1..numberOfAvailableMoves
        return Utils.IndexOfRthZero(board, r);
    }

    public void Move(XO xo, int row, int col)
    {
        var move = col + 3 * row;
        var vec = xo == XO.X
            ? xs |= 1 << move
            : os |= 1 << move;

        // Runtime.Assert((xs & os) == 0, "(xs & os) == 0");
        // Runtime.Assert((xs & ~FULL) == 0, "(xs & ~FULL) == 0");
        // Runtime.Assert((os & ~FULL) == 0, "(os & ~FULL) == 0");

        if ((rowTerminals[row] is var r && (vec & r) == r) ||
            (colTerminals[col] is var c && (vec & c) == c) ||
            (vec & 0b100010001) == 0b100010001 ||
            (vec & 0b001010100) == 0b001010100)
            winner = (OptionalWinner)xo;
        else if ((xs | os) == FULL)
            winner = OptionalWinner.Draw;
    }

    public void MoveToRowCol(int move, out int row, out int col)
    {
        row = move / 3;
        col = move % 3;
    }

    public int RowColToMove(int row, int col)
        => row * 3 + col;

    public void Save()
    {
        xsCache = xs;
        osCache = os;
        winnerCache = winner;
    }

    public void Restore()
    {
        xs = xsCache;
        os = osCache;
        winner = winnerCache;
    }

    public Board Copy() => this;
}

internal enum OptionalWinner
{
    Empty,
    O,
    Draw,
    X,
}

internal enum Winner
{
    O = 1,
    Draw,
    X,
}

internal enum XO
{
    O = 1,
    X = 3,
}

internal static class Utils
{
    public static void Shuffle<T>(this Random rng, ImmutableArray<T>.Builder array)
    {
        int n = array.Count;
        while (n > 1)
        {
            int k = rng.Next(n--);
            T temp = array[n];
            array[n] = array[k];
            array[k] = temp;
        }
    }

    /// <summary>
    /// Get the index of the <paramref name="r"/>-th zero from right.
    /// </summary>
    /// <param name="board">9-bit mask.</param>
    /// <param name="r">1..9</param>
    /// <returns>0..8</returns>
    public static int IndexOfRthZero(int board, int r)
    {
        var rs = (1 << r) - 1; // 1^r
        var move = 0;

        // get the index of the r-th zero (from the least-significant bit)

        ////for (int k = 0; k < 9; k++)
        ////{
        ////    rs >>= 1 - (board & 1); // remove '1' from rs when board has `0`
        ////    board >>= 1; // move board
        ////    move += rs & 1; // increment move when rs is not empty
        ////}

        rs >>= 1 - (board & 1); // 1st loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 2nd loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 3rd loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 4th loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 5th loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 6th loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 7th loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 8th loop
        board >>= 1;
        move += rs & 1;

        rs >>= 1 - (board & 1); // 9th loop
        move += rs & 1;

        return move;
    }

    public static void SimulationTimes<T>(IAgent<T> agent, int times)
        where T : IBoard<T>
    {
        var sw = new Stopwatch();
        var firstTurnAverage = 0L;
        var firstTurnMax = 0L;
        var otherTurnAverage = 0L;
        var otherTurnMax = 0L;
        for (int i = 0; i < times; i++)
        {
            sw.Start();
            var gcb = GC.TryStartNoGCRegion(244_000_000);
            // Runtime.Assert(gcb, "TryStartNoGCRegion");

            agent.Init();
            var initTime = sw.ElapsedMilliseconds;
            sw.Restart();
            agent.MyMove(out _, out _);
            var turnTime = sw.ElapsedMilliseconds;
            sw.Reset();

            GC.EndNoGCRegion();
            // GC.Collect();

            var firstTurn = initTime + turnTime;

            if (firstTurnMax < firstTurn)
                firstTurnMax = firstTurn;
            if (otherTurnMax < turnTime)
                otherTurnMax = turnTime;

            firstTurnAverage += firstTurn;
            otherTurnAverage += turnTime;

            // Console.WriteLine($"so far: max={firstTurnMax} ; max={otherTurnMax}");
        }
        firstTurnAverage /= times;
        otherTurnAverage /= times;
        Console.WriteLine($"1st turn: avg={firstTurnAverage}ms ; max={firstTurnMax}");
        Console.WriteLine($"2.. turn: avg={otherTurnAverage}ms ; max={otherTurnMax}");
    }

    public static void Duels(int duels)
    {
        var draws = 0;
        var wins = 0;
        var d = duels;
        var sw = new Stopwatch();
        while (d-- > 0)
        {
            for (int i = 0; i < 2; i++)
            {
                var a1 = new MCTS<ComplexBoard>(new ComplexBoard());
                // var a2 = new MCTS<ComplexBoard>(new ComplexBoard());
                var a2 = new FlatMC<ComplexBoard>(new ComplexBoard());

                sw.Start();
                // var gcb = GC.TryStartNoGCRegion(244_000_000);
                // Runtime.Assert(gcb, "TryStartNoGCRegion");
                var result = i == 0
                    ? Duel(a1, a2)
                    : Duel(a2, a1);
                Console.WriteLine($"Duel #{duels - d}.{i} : {result:+0;-#} : {sw.ElapsedMilliseconds}ms");
                sw.Reset();
                // GC.EndNoGCRegion();
                GC.Collect();

                if (result == 0)
                    draws++;
                else if (result == (i == 0 ? 1 : -1))
                    wins++;
            }
        }
        duels *= 2;
        var loss = duels - wins - draws;

        Console.WriteLine($"a1 : {wins} / {duels} = {(double)wins / duels}");
        Console.WriteLine($"a2 : {loss} / {duels} = {(double)loss / duels}");
    }

    private static int Duel(IAgent<ComplexBoard> a1, IAgent<ComplexBoard> a2)
    {
        Winner winner;
        var a = a1;
        while (!a.Board.IsTerminal(out winner))
        {
            a1.MyMove(out var a1Row, out var a1Col);
            a2.OpponentMove(a1Row, a1Col);
            var tmp = a1;
            a1 = a2;
            a2 = tmp;
        }
        return (int)winner - 2; // +1 when a1 won, -1 when lost, 0 on draw
    }
}

internal static class Runtime
{
    public static Random random = new Random();

    internal static void Assert(bool b, string s)
    {
        if (!b)
        {
            _ = 1;
            throw new InvalidOperationException("Assert failed: " + s);
        }
    }
}

internal static class Parameters
{
    public const int SIMULATIONS = 400;
    public const double C = 1.41;
    public const int MCTS_MS = 100 - 5;
    public const int MCTS_WARMUP_MS = 1000 - 50 - MCTS_MS;
    public const int MCTS_STEPS = 9 * 400;
    public const int MCTS_WARMUP_STEPS = 9 * 4000 - MCTS_STEPS;
}