

#include "MoveGeneration.h"
#include "Evaluation.h"
#include "Board.h"
#include "Search.h"
#include "const.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

std::vector<std::string> option_name = {
    "RFP_MULTIPLIER",
    "RFP_IMPROVING_MULTIPLIER",
    "RFP_BASE",
    "RFP_IMPROVING_BASE",
    "LMP_BASE",
    "LMP_MULTIPLIER",
    "PVS_QUIET_BASE",
    "PVS_QUIET_MULTIPLIER",
    "PVS_NOISY_BASE",
    "PVS_NOISY_MULTIPLIER",
    "HISTORY_BASE",
    "HISTORY_MULTIPLIER",
    "ASP_WINDOW_INITIAL",
    "ASP_WINDOW_MAX",
    "PAWN_CORRHIST_MULTIPLIER",
    "MINOR_CORRHIST_MULTIPLIER",
    "NONPAWN_CORRHIST_MULTIPLIER",
    "QS_SEE_PRUNING_MARGIN",
    "HISTORY_PRUNING_MULTIPLIER",
    "HISTORY_PRUNING_BASE",
    "HISTORY_LMR_MULTIPLIER",
    "HISTORY_LMR_BASE",
    "NMP_EVAL_DIVISER",
    "NMP_DEPTH_DIVISER",
    "MAX_NMP_EVAL_R"
};

std::vector<int> option_base = {
    85,
    60,
    -49,
    -49,
    0,
    1,
    0,
    63,
    -1,
    18,
    4,
    2,
    40,
    300,
    5,
    5,
    5,
    0,
    50,
    0,
    25,
    0,
    400,
    3,
    3
};


std::vector<int> option_min = { 
    50,
    30, 
    -100, 
    -100, 
    -1, 
    1,
    -70,
    20,
    -70, 
    5,
    0, 
    1,
    20, 
    100,
    1,
    1,
    1,
    -150,
    10,
    -200,
    10, 
    -100,
    200,
    2,
    2
};
std::vector<int> option_max = {
    160,
    150,
    100,
    100,
    6,
    3,
    100,
    150,
    50,
    80,
    20,
    10,
    100,
    1000,
    10,
    10,
    10,
    100,
    200,
    100,
    80,
    100,
    800,
    7,
    4
};

std::vector<int*> option_var = {
    &RFP_MULTIPLIER,
    &RFP_IMPROVING_MULTIPLIER,
    &RFP_BASE,
    &RFP_IMPROVING_BASE,
    &LMP_BASE,
    &LMP_MULTIPLIER,
    &PVS_QUIET_BASE,
    &PVS_QUIET_MULTIPLIER,
    &PVS_NOISY_BASE,
    &PVS_NOISY_MULTIPLIER,
    &HISTORY_BASE,
    &HISTORY_MULTIPLIER,
    &ASP_WINDOW_INITIAL,
    &ASP_WINDOW_MAX,
    &PAWN_CORRHIST_MULTIPLIER,
    &MINOR_CORRHIST_MULTIPLIER,
    &NONPAWN_CORRHIST_MULTIPLIER,
    &QS_SEE_PRUNING_MARGIN,
    &HISTORY_PRUNING_MULTIPLIER,
    &HISTORY_PRUNING_BASE,
    &HISTORY_LMR_MULTIPLIER,
    &HISTORY_LMR_BASE,
    &NMP_EVAL_DIVISER,
    &NMP_DEPTH_DIVISER,
    &MAX_NMP_EVAL_R
};



/*int RFP_MULTIPLIER = 75;
int RFP_BASE = 0;

int LMP_BASE = 1;
int LMP_MULTIPLIER = 3;

int PVS_QUIET_BASE = 0;
int PVS_QUIET_MULTIPLIER = 70;

int PVS_NOISY_BASE = 0;
int PVS_NOISY_MULTIPLIER = 20;

int HISTORY_BASE = 0;
int HISTORY_MULTIPLIER = 1;
*/

// generate 32-bit pseudo legal numbers




const std::string start_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::string kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ";

std::vector<std::string> position_commands = { "position", "startpos", "fen", "moves" };
std::vector<std::string> go_commands = { "go", "movetime", "wtime", "btime", "winc", "binc", "movestogo" };
std::vector<std::string> option_commands = { "setoption", "name",  "value" };
std::vector<std::string> datagen_commands = { "datagen", "pos", "file" };
std::vector<std::string> filter_commands = { "filter", "input", "output"};

Board main_board;

int perft_depth;
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    const auto start = str.find_first_not_of(whitespace);
    const auto end = str.find_last_not_of(whitespace);

    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }

    return str.substr(start, end - start + 1);
}

std::string TryGetLabelledValue(const std::string& text, const std::string& label, const std::vector<std::string>& allLabels, const std::string& defaultValue = "") {
    
    // Trim leading and trailing whitespace
    std::string trimmedText = trim(text);

    // Find the position of the label in the trimmed text
    size_t labelPos = trimmedText.find(label);
    if (labelPos != std::string::npos) {
        // Determine the start position of the value
        size_t valueStart = labelPos + label.length();
        size_t valueEnd = trimmedText.length();

        // Iterate through allLabels to find the next label position
        for (const std::string& otherID : allLabels) {
            if (otherID != label) {
                size_t otherIDPos = trimmedText.find(otherID, valueStart);
                if (otherIDPos != std::string::npos && otherIDPos < valueEnd) {
                    valueEnd = otherIDPos;
                }
            }
        }

        // Extract the value and trim leading/trailing whitespace
        std::string value = trimmedText.substr(valueStart, valueEnd - valueStart);
        return trim(value);
    }

    return defaultValue;
}
int TryGetLabelledValueInt(const std::string& text, const std::string& label, const std::vector<std::string>& allLabels, int defaultValue = 0)
{
    // Helper function TryGetLabelledValue should be implemented as shown earlier
    std::string valueString = TryGetLabelledValue(text, label, allLabels, std::to_string(defaultValue));

    // Extract the first part of the valueString up to the first space
    std::istringstream iss(valueString);
    std::string firstPart;
    iss >> firstPart;

    // Try converting the extracted string to an integer
    try
    {
        return std::stoi(firstPart);
    }
    catch (const std::invalid_argument& e)
    {
        // If conversion fails, return the default value
        return defaultValue;
    }
}
std::vector<std::string> splitStringBySpace(const std::string& str)
{
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;

    while ((end = str.find(' ', start)) != std::string::npos)
    {
        if (end != start)
        { // Ignore multiple consecutive spaces
            tokens.push_back(str.substr(start, end - start));
        }
        start = end + 1;
    }

    // Push the last token if it's not empty
    if (start < str.size())
    {
        tokens.push_back(str.substr(start));
    }

    return tokens;
}

static uint64_t Perft(Board& board, int depth)
{

    if (depth == 0)
    {
        return 1ULL;
    }

    MoveList move_list;


    uint64_t nodes = 0;

    Generate_Legal_Moves(move_list, board, false);

    for (int i = 0; i < move_list.count; ++i)
    {
        Move& move = move_list.moves[i];
        int lastEp = board.enpassent;
        uint64_t lastCastle = board.castle;
        int lastside = board.side;
        int captured_piece = board.mailbox[move.To];

        uint64_t last_zobrist = board.zobristKey;
        uint64_t last_pawnKey = board.PawnKey;
        uint64_t last_minorKey = board.MinorKey;
        uint64_t last_whiteNPKey = board.WhiteNonPawnKey;
        uint64_t last_blackNPKey = board.BlackNonPawnKey;
        
        MakeMove(board, move);


        uint64_t blackNPKey_from_scratch = generate_BlackNP_Hash(board);
        if (board.BlackNonPawnKey != blackNPKey_from_scratch)
        {
            std::cout << "CRITICAL ERROR: black np key doesn't match\n";
            printMove(move);
            std::cout << "\n\n";
        }
        if (isLegal(move, board))
        {
            uint64_t nodes_added = Perft(board, depth - 1);
            nodes += nodes_added;
        }


        UnmakeMove(board, move, captured_piece);

        board.enpassent = lastEp;
        board.castle = lastCastle;
        board.side = lastside;
        board.zobristKey = last_zobrist;
        board.PawnKey = last_pawnKey;
        board.MinorKey = last_minorKey;
        board.WhiteNonPawnKey = last_whiteNPKey;
        board.BlackNonPawnKey = last_blackNPKey;

    }
    return nodes;
}

int Calculate_Hard_Bound(int time, int incre)
{
    return time / 2;
}
int Calculate_Soft_Bound(int time, int incre)
{
    return  0.6 * (static_cast<float>(time) / static_cast<float>(20) + static_cast<float>(incre) * static_cast<float>(3) / static_cast<float>(4));
}
void Initialize_TT(int size)
{
    //std::cout <<"size" << size << "\n";
    uint64_t bytes = static_cast<uint64_t>(size) * 1024ULL * 1024ULL;

    //std::cout << bytes<<"\n";
    TTSize = bytes / sizeof(TranspositionEntry);

    if (TTSize % 2 != 0)
    {
        TTSize -= 1;
    }

    if (TranspositionTable)
        delete [] TranspositionTable;

    TranspositionTable = new TranspositionEntry[TTSize]();
    
}
void ProcessUCI(std::string input)
{
    //std::cout << (input) << "\n";
    //std::string input = "This  is   a  sample string";
    std::vector<std::string> Commands = splitStringBySpace(input);
    std::string main_command = Commands[0];

    if (main_command == "uci")
    {
        
        std::cout << "id name Turbulence_v4 v0.0.4" << "\n";;
        std::cout << "id author ksw0518" << "\n";;
        std::cout << "\n";
        std::cout << "option name Threads type spin default 1 min 1 max 1\n";
        std::cout << "option name Hash type spin default 12 min 1 max 4096\n";
        //for (int i = 0; i < option_name.size(); i++)//for spsa
        //{
        //    std::cout << "option name " << option_name[i];
        //    std::cout << " type spin ";
        //    std::cout << " default " << option_base[i];
        //    std::cout << " min " << option_min[i];
        //    std::cout << " max " << option_max[i];
        //    std::cout << "\n";
        //}
        isPrettyPrinting = false;
        
        std::cout << "uciok" << "\n";
    }
    else if (main_command == "datagen")
    {
        int pos = TryGetLabelledValueInt(input, "pos", datagen_commands);
        std::string file = TryGetLabelledValue(input, "file", datagen_commands);
        
        Datagen(pos, file);
    }
    else if (main_command == "filter")
    {
        //std::cout << "fuck";
        //std::cout << input;
        std::string input_file = TryGetLabelledValue(input, "input", filter_commands);
        std::string output_file = TryGetLabelledValue(input, "output", filter_commands);
        //std::cout << "data processed";
        filterData(input_file, output_file);
    }
    else if (main_command == "ucinewgame")
    {
        
        Initialize_TT(16);
        initializeLMRTable();
        isPrettyPrinting = false;
    }
    else if (main_command == "setoption")
    {
        std::string option = TryGetLabelledValue(input, "name", option_commands);
        int value = TryGetLabelledValueInt(input, "value", option_commands);
       
        if (option == "Hash")
        {
            Initialize_TT(value);
        }
        else
        {
            for (size_t i = 0; i < option_name.size(); i++)
            {
                if (option == option_name[i])
                {
                    *option_var[i] = value;
                }
            }
        }

    }
    else if (main_command == "isready")
    {
        std::cout << "readyok"<< "\n";

    }
    else if (main_command == "quit")
    {
        exit(0);
    }
    else if (main_command == "position")
    {
        main_board.history.clear();
        if (Commands[1] == "startpos")
        {
            if (Commands.size() == 2)
            {

                parse_fen(start_pos, main_board);
                main_board.zobristKey = generate_hash_key(main_board);
                main_board.history.push_back(main_board.zobristKey);
            }
            else
            {
                parse_fen(start_pos, main_board);
                main_board.zobristKey = generate_hash_key(main_board);
                main_board.history.push_back(main_board.zobristKey);

                std::string moves_in_string = TryGetLabelledValue(input, "moves", position_commands);
                if (moves_in_string != "") // move is not empty
                {
                    std::vector<std::string> moves_seperated = splitStringBySpace(moves_in_string);
                    MoveList moveList;

                    for (size_t i = 0; i < moves_seperated.size(); i++)
                    {
                        std::string From = std::string(1, moves_seperated[i][0]) + std::string(1, moves_seperated[i][1]);
                        std::string To = std::string(1, moves_seperated[i][2]) + std::string(1, moves_seperated[i][3]);
                        std::string promo = "";
                        
                        if (moves_seperated[i].size() > 4)
                        {
                            promo = std::string(1, (moves_seperated[i][4]));

                        }
                        Move move_to_play;
                        move_to_play.From = GetSquare(From);
                        move_to_play.To = GetSquare(To);

                        moveList.clear();
                        Generate_Legal_Moves(moveList, main_board, false);

                        for (size_t j = 0; j < moveList.count; j++)
                        {
   

                            if ((move_to_play.From == moveList.moves[j].From) && (move_to_play.To == moveList.moves[j].To)) //found same move
                            {



                                if ((moveList.moves[j].Type & knight_promo) != 0) // promo
                                {
                                    //std::cout << "promo" << "\n";
                                    //std::cout << promo << "\n";
                                    if (promo == "q")
                                    {
                                        //std::cout << "qpromo";
                                        if ((moveList.moves[j].Type == queen_promo) || (moveList.moves[j].Type == queen_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;

                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                    else if (promo == "r")
                                    {
                                        if ((moveList.moves[j].Type == rook_promo) || (moveList.moves[j].Type == rook_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;
                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                    else if (promo == "b")
                                    {
                                        if ((moveList.moves[j].Type == bishop_promo) || (moveList.moves[j].Type == bishop_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;
                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                    else if (promo == "n")
                                    {
                                        if ((moveList.moves[j].Type == knight_promo) || (moveList.moves[j].Type == knight_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;
                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                }
                                else
                                {

                                    MakeMove(main_board, moveList.moves[j]);

                                    main_board.halfmove++;


                                    break;
                                }



                            }
                        }
                    
                    }

                }
            }
           
        }
        else if (Commands[1] == "fen")
        {
            std::string fen = TryGetLabelledValue(input, "fen", position_commands);
            std::string moves = TryGetLabelledValue(input, "moves", position_commands);

            if (moves == "")
            {
                parse_fen(fen, main_board);
                main_board.zobristKey = generate_hash_key(main_board);
            
                main_board.history.push_back(main_board.zobristKey);

            }
            else
            {
                parse_fen(fen, main_board);
                main_board.zobristKey = generate_hash_key(main_board);
                main_board.history.push_back(main_board.zobristKey);
                std::string moves_in_string = TryGetLabelledValue(input, "moves", position_commands);
                if (moves_in_string != "") // move is not empty
                {
                    std::vector<std::string> moves_seperated = splitStringBySpace(moves_in_string);
                    MoveList moveList;


                    for (size_t i = 0; i < moves_seperated.size(); i++)
                    {
                        std::string From = std::string(1, moves_seperated[i][0]) + std::string(1, moves_seperated[i][1]);
                        std::string To = std::string(1, moves_seperated[i][2]) + std::string(1, moves_seperated[i][3]);
                        std::string promo = "";
                        if (moves_seperated[i].size() > 4)
                        {
                            promo = std::string(1, (moves_seperated[i][4]));
                        }
                        Move move_to_play;
                        move_to_play.From = GetSquare(From);
                        move_to_play.To = GetSquare(To);

                        //std::cout << CoordinatesToChessNotation(move_to_play.From);
                        //std::cout << CoordinatesToChessNotation(move_to_play.To);
                        //std::cout << promo;
                        moveList.clear();
                        Generate_Legal_Moves(moveList, main_board, false);

                        for (size_t j = 0; j < moveList.count; j++)
                        {
                            //Console.WriteLine("12");
                            //nodes = 0;

                            if ((move_to_play.From == moveList.moves[j].From) && (move_to_play.To == moveList.moves[j].To)) //found same move
                            {



                                if ((moveList.moves[j].Type & knight_promo) != 0) // promo
                                {
                                    if (promo == "q")
                                    {
                                        if ((moveList.moves[j].Type == queen_promo) || (moveList.moves[j].Type == queen_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;

                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                    else if (promo == "r")
                                    {
                                        if ((moveList.moves[j].Type == rook_promo) || (moveList.moves[j].Type == rook_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;
                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                    else if (promo == "b")
                                    {
                                        if ((moveList.moves[j].Type == bishop_promo) || (moveList.moves[j].Type == bishop_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;
                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                    else if (promo == "n")
                                    {
                                        if ((moveList.moves[j].Type == knight_promo) || (moveList.moves[j].Type == knight_promo_capture))
                                        {
                                            MakeMove(main_board, moveList.moves[j]);
                                            break;
                                            //Move_to_do.Add(moveList[j]);
                                        }
                                    }
                                }
                                else
                                {

                                    MakeMove(main_board, moveList.moves[j]);


                                    break;
                                }



                            }
                        }

                    }

                }
            }
            //std::cout << TryGetLabelledValue(input, "fen", position_commands);
        }
        else if ((Commands[1] == "kiwi"))
        {
            parse_fen(kiwipete, main_board);
            main_board.zobristKey = generate_hash_key(main_board);
            main_board.history.push_back(main_board.zobristKey);
        }
        //std::cout << generate_Pawn_Hash(main_board);

    }
    else if (main_command == "perft")
    {
        if (Commands[0] == "perft")
        {

            perft_depth = std::stoi(Commands[1]);

            //std::cout << (perft_depth);
            auto start = std::chrono::high_resolution_clock::now();

            uint64_t nodes = Perft(main_board, perft_depth);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::milli> elapsedMS = end - start;
            //std::chrono::duration<double, std::milli> elapsedS = end - start;

            float second = elapsedMS.count() / 1000;

            double nps = nodes / second;

            std::cout << (nodes);
            std::cout << "\n";


        }
        }
    else if (main_command == "go")
    {
        if (Commands[1] == "depth")
        {
            //Negamax_nodecount = 0;
            if (Commands.size() == 3)
            {
                int depth = std::stoi(Commands[2]);
                IterativeDeepening(main_board, depth);

            }
            else if (Commands.size() > 3)
            {
                if (Commands[3] == "root")
                {
                    int depth = std::stoi(Commands[2]);
                    IterativeDeepening(main_board, depth, -1, true);
                }
            }

        }
        else if (Commands[1] == "nodes")
        {
                int node = std::stoi(Commands[2]);
                IterativeDeepening(main_board, 99, -1, false, true, -1, -1, -1, node, node);
        }
        else if (Commands[1] == "movetime")
        {
            int movetime = std::stoi(Commands[2]);
            IterativeDeepening(main_board, 99, movetime);
        }
        else if (Commands[1] == "wtime")
        {
            int depth = TryGetLabelledValueInt(input, "depth", go_commands);
            int wtime = TryGetLabelledValueInt(input, "wtime", go_commands);
            int btime = TryGetLabelledValueInt(input, "btime", go_commands);
            int winc = TryGetLabelledValueInt(input, "winc", go_commands);
            int binc = TryGetLabelledValueInt(input, "binc", go_commands);
           

            if (depth != 0)
            {
                //int depth = std::stoi(Commands[2]);
                IterativeDeepening(main_board, depth);
                
            }
            else
            {
                int hard_bound;
                int soft_bound;
                int baseTime = 0;
                int maxTime = 0;
                if (main_board.side == White)
                {
                    hard_bound = Calculate_Hard_Bound(wtime, winc);
                    soft_bound = Calculate_Soft_Bound(wtime, winc);
                    baseTime = wtime * DEF_TIME_MULTIPLIER + winc * DEF_INC_MULTIPLIER;
                    maxTime = std::max(1.00, wtime * MAX_TIME_MULTIPLIER);
                }
                else
                {
                    hard_bound = Calculate_Hard_Bound(btime, binc);
                    soft_bound = Calculate_Soft_Bound(btime, binc);
                    baseTime = btime * DEF_TIME_MULTIPLIER + binc * DEF_INC_MULTIPLIER;
                    maxTime = std::max(1.00, btime * MAX_TIME_MULTIPLIER);
                }

                IterativeDeepening(main_board, 99, hard_bound, false, true, soft_bound, baseTime, maxTime);
            }
           
        }
        else
        {
            IterativeDeepening(main_board, 99);
        }
        //else if (Commands[1] == "perft")
        //{

        //}
    }
    else if (main_command == "show")
    {
        PrintBoards(main_board);
        print_mailbox(main_board.mailbox);
    }
    else if (main_command == "eval")
    {
        std::cout << ("evaluation: ") << Evaluate(main_board) << "cp ";
        if (main_board.side == White)
        {
            std::cout << ("(White's perspective)\n");
        }
        else
        {
            std::cout << ("(Black's perspective)\n");


            std::cout << ("White's perspective: ") << -Evaluate(main_board) << "cp \n";
        }



    }
    else if(main_command == "move")
    {
        std::string From = std::string(1, Commands[1][0]) + std::string(1, Commands[1][1]);
        std::string To = std::string(1, Commands[1][2]) + std::string(1, Commands[1][3]);
        std::string promo = "";
        if (Commands[1].size() > 4)
        {
            promo = std::string(1, (Commands[1][4]));
        }
        Move move_to_play;
        move_to_play.From = GetSquare(From);
        move_to_play.To = GetSquare(To);

        //std::cout << CoordinatesToChessNotation(move_to_play.From);
        //std::cout << CoordinatesToChessNotation(move_to_play.To);
        //std::cout << promo;

        MoveList moveList;
        moveList.clear();
        Generate_Legal_Moves(moveList, main_board, false);

        for (size_t j = 0; j < moveList.count; j++)
        {
            //Console.WriteLine("12");
            //nodes = 0;

            if ((move_to_play.From == moveList.moves[j].From) && (move_to_play.To == moveList.moves[j].To)) //found same move
            {



                if ((moveList.moves[j].Type & knight_promo) != 0) // promo
                {
                    if (promo == "q")
                    {
                        if ((moveList.moves[j].Type == queen_promo) || (moveList.moves[j].Type == queen_promo_capture))
                        {
                            MakeMove(main_board, moveList.moves[j]);
                            break;

                            //Move_to_do.Add(moveList[j]);
                        }
                    }
                    else if (promo == "r")
                    {
                        if ((moveList.moves[j].Type == rook_promo) || (moveList.moves[j].Type == rook_promo_capture))
                        {
                            MakeMove(main_board, moveList.moves[j]);
                            break;
                            //Move_to_do.Add(moveList[j]);
                        }
                    }
                    else if (promo == "b")
                    {
                        if ((moveList.moves[j].Type == bishop_promo) || (moveList.moves[j].Type == bishop_promo_capture))
                        {
                            MakeMove(main_board, moveList.moves[j]);
                            break;
                            //Move_to_do.Add(moveList[j]);
                        }
                    }
                    else if (promo == "n")
                    {
                        if ((moveList.moves[j].Type == knight_promo) || (moveList.moves[j].Type == knight_promo_capture))
                        {
                            MakeMove(main_board, moveList.moves[j]);
                            break;
                            //Move_to_do.Add(moveList[j]);
                        }
                    }
                }
                else
                {
                    //Console.WriteLine(MoveType[moveList[j].Type]);
                    //Console.WriteLine(ascii_pieces[moveList[j].Piece]);
                   // printMove(moveList[j]);
                    //Console.Write(" ");
                    //Move_to_do.Add(moveList[j]); 
                    MakeMove(main_board, moveList.moves[j]);
                    //if (isMoveIrreversible(moveList[j]))
                    //{
                    //    //Console.WriteLine("aaa");
                    //    Repetition_table.Clear();
                    //    main_board.halfmove = 0;
                    //}
                    //Repetition_table.Add(main_Zobrist);
                    main_board.halfmove++;


                    break;
                }



            }
        }

        //main_board.history.push_back(main_board.)

        uint64_t hash_debug = generate_hash_key(main_board);

        PrintBoards(main_board);
        if (hash_debug != main_board.zobristKey)
        {
            std::cout << "warning:zobrist key doesn't match";
        }
    }
    else if(main_command == "bench")
    {
        bench();
    }

}


static void InitAll()
{
    InitializeBetweenTable();
    InitializeLeaper();
    init_sliders_attacks(1);
    init_sliders_attacks(0);
    init_tables();
    init_random_keys();
    
}
int main(int argc, char* argv[])
{

    initializeLMRTable();
    std::cout.sync_with_stdio(false);
    InitAll();
    
    parse_fen(start_pos, main_board);
    main_board.zobristKey = generate_hash_key(main_board);
    main_board.history.push_back(main_board.zobristKey);

    Initialize_TT(16);

    uint64_t hash_key = 0ULL;
    //Datagen();
    if (argc > 1) {
        std::string command = argv[1]; // First argument (after program name)

        if (command == "bench") {

            bench();
            return EXIT_SUCCESS;
        }
    }
    while (true)
    {
        std::string input;

        std::getline(std::cin, input);
        if (input != "")
        {
            ProcessUCI(input);
        }

    }

    

}
