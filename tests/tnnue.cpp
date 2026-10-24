#include <gtest/gtest.h>

#include <cfloat>
#include <cmath>

#include "NNUE.hpp"
#include "engine.hpp"
#include "gameboard.hpp"
#include "minimax.hpp"

namespace {

using namespace ShumiChess;

constexpr int square_e5 = GameBoard::square_e5;

TEST(NNUEData, DefaultConstructionProducesExactZero)
{
    NNUE_data nnue;
    GameBoard board;

    EXPECT_EQ(nnue.Calculate_nnue_positional(board), 0.0f);
    board.turn = Color::BLACK;
    EXPECT_EQ(nnue.Calculate_nnue_positional(board), 0.0f);

    const GameBoard endgame("7k/8/8/4N3/8/8/8/K7 w - - 0 1");
    EXPECT_EQ(nnue.Calculate_nnue_positional(endgame), 0.0f);
}

TEST(NNUEData, OwnsSuppliedParametersAndRefreshesBoardInputs)
{
    NNUEParameters parameters{};
    parameters.Neurons[0].piece_weight[1][square_e5] = 3.0f;
    parameters.Neurons[0].turn_weight = 2.0f;
    parameters.Neurons[0].bias = -1.0f;
    parameters.output_weight[0] = 4.0f;
    parameters.output_bias = 1.0f;

    NNUE_data nnue(parameters);
    parameters.Neurons[0].piece_weight[1][square_e5] = 100.0f;

    // Score = 1 + 4 * max(0, 3 * white_knight_on_e5 + 2 * black_to_move - 1).
    const struct {
        const char* fen;
        float expected;
    } cases[] = {
        {"7k/8/8/4N3/8/8/8/K7 w - - 0 1", 9.0f},
        {"7k/8/8/4N3/8/8/8/K7 b - - 0 1", 17.0f},
        {"7k/8/8/3N4/8/8/8/K7 w - - 0 1", 1.0f},
        {"7k/8/8/4n3/8/8/8/K7 w - - 0 1", 1.0f},
        {"7k/8/8/8/8/8/8/K7 b - - 0 1", 5.0f},
        {"7k/8/8/4N3/8/8/8/K7 w - - 0 1", 9.0f},
    };

    for (const auto& test_case : cases) {
        const GameBoard board(test_case.fen);
        EXPECT_EQ(nnue.Calculate_nnue_positional(board), test_case.expected)
            << test_case.fen;
    }
}

TEST(NNUEData, ReplacesAndZerosOwnedParameters)
{
    NNUE_data nnue;
    NNUEParameters parameters{};
    const int unit = NNUE_NEURON_COUNT - 1;
    parameters.Neurons[unit].piece_weight[11][63] = 2.0f;
    parameters.Neurons[unit].turn_weight = 3.0f;
    parameters.Neurons[unit].bias = 1.0f;
    parameters.output_weight[unit] = 4.0f;
    parameters.output_bias = 5.0f;

    const GameBoard board("k7/8/8/8/8/8/8/7K b - - 0 1");

    nnue.set_parameters(parameters);
    parameters.output_bias = 100.0f;
    EXPECT_EQ(nnue.Calculate_nnue_positional(board), 29.0f);

    nnue.zero_parameters();
    EXPECT_EQ(nnue.Calculate_nnue_positional(board), 0.0f);
    const auto& zeroed = nnue.get_parameters();
    EXPECT_EQ(zeroed.Neurons[unit].piece_weight[11][63], 0.0f);
    EXPECT_EQ(zeroed.Neurons[unit].turn_weight, 0.0f);
    EXPECT_EQ(zeroed.Neurons[unit].bias, 0.0f);
    EXPECT_EQ(zeroed.output_weight[unit], 0.0f);
    EXPECT_EQ(zeroed.output_bias, 0.0f);
}

TEST(NNUEData, TrainingUsesAndUpdatesOwnedParameters)
{
    NNUEParameters parameters{};
    parameters.Neurons[0].piece_weight[1][square_e5] = 3.0f;
    parameters.output_weight[0] = 2.0f;
    NNUE_data nnue(parameters);

    const GameBoard board("7k/8/8/4N3/8/8/8/K7 w - - 0 1");
    NNUETrainingExample example{};
    nnue.fill_nnue_inputs(board, example.inputs);
    // Positional target = search score minus material score.
    example.target_positional_score_cp = 110.0f - 100.0f;

    NNUEJacobianRow row{};
    nnue.calculate_nnue_jacobian_row(example.inputs, row);
    EXPECT_EQ(row.Neurons[0].piece_weight[1][square_e5], 2.0f);
    EXPECT_EQ(row.output_weight[0], 3.0f);
    EXPECT_EQ(row.output_bias, 1.0f);

    NNUEGradient gradient{};
    const float before = nnue.calculate_nnue_gradient(example, gradient);
    EXPECT_EQ(before, -4.0f);
    EXPECT_EQ(gradient.Neurons[0].piece_weight[1][square_e5], -8.0f);
    EXPECT_EQ(gradient.output_weight[0], -12.0f);
    EXPECT_EQ(gradient.output_bias, -4.0f);
    EXPECT_EQ(nnue.Calculate_nnue_positional(board), 6.0f);

    nnue.apply_nnue_gradient(gradient, 0.01f);
    const float after = nnue.calculate_nnue_gradient(example, gradient);
    EXPECT_LT(std::fabs(after), std::fabs(before));
    EXPECT_FLOAT_EQ(nnue.get_parameters().Neurons[0].piece_weight[1][square_e5], 3.08f);
    EXPECT_EQ(parameters.Neurons[0].piece_weight[1][square_e5], 3.0f);
}

TEST(NNUEData, MinimaxIncludesScoreOnceWithCorrectPerspective)
{
    Engine engine;
    MinimaxAI minimax(engine);

    // The default network contributes exactly zero to the existing evaluation.
    EXPECT_EQ(minimax.evaluate_board_t<Color::WHITE>(NNUE),
              minimax.evaluate_board_t<Color::WHITE>(SLUG));
    EXPECT_EQ(minimax.evaluate_board_t<Color::BLACK>(NNUE),
              minimax.evaluate_board_t<Color::BLACK>(SLUG));

    NNUEParameters parameters{};
    parameters.Neurons[0].piece_weight[1][square_e5] = 3.0f;
    parameters.Neurons[0].turn_weight = 2.0f;
    parameters.Neurons[0].bias = -1.0f;

    const struct {
        const char* fen;
        int positional_cp;
    } cases[] = {
        {"7k/8/8/4N3/8/8/8/K7 w - - 0 1", 9},
        {"7k/8/8/4N3/8/8/8/K7 b - - 0 1", 17},
        {"7k/8/8/3N4/8/8/8/K7 w - - 0 1", 1},
        {"7k/8/8/8/8/8/8/K7 b - - 0 1", 5},
    };

    for (const int sign : {1, -1}) {
        parameters.output_weight[0] = 4.0f * sign;
        parameters.output_bias = 1.0f * sign;
        minimax.nnue.set_parameters(parameters);

        for (const auto& test_case : cases) {
            SCOPED_TRACE(test_case.fen);
            SCOPED_TRACE(sign);
            engine.reset_engine(test_case.fen);
            const int baseline_white = minimax.evaluate_board_t<Color::WHITE>(SLUG);
            const int baseline_black = minimax.evaluate_board_t<Color::BLACK>(SLUG);
            const int positional_cp = sign * test_case.positional_cp;

            EXPECT_EQ(minimax.evaluate_board_t<Color::WHITE>(NNUE),
                      baseline_white + positional_cp);
            EXPECT_EQ(minimax.evaluate_board_t<Color::BLACK>(NNUE),
                      baseline_black - positional_cp);
        }
    }

    // Fractional network scores are rounded to the evaluator's integer centipawns.
    parameters = {};
    for (const int sign : {1, -1}) {
        parameters.output_bias = 1.5f * sign;
        minimax.nnue.set_parameters(parameters);
        EXPECT_EQ(minimax.evaluate_board_t<Color::WHITE>(NNUE),
                  minimax.evaluate_board_t<Color::WHITE>(SLUG) + 2 * sign);
        EXPECT_EQ(minimax.evaluate_board_t<Color::BLACK>(NNUE),
                  minimax.evaluate_board_t<Color::BLACK>(SLUG) - 2 * sign);
    }
}

TEST(NNUEData, TrainingOnePositionConverges)
{
    constexpr int TRAINING_ITERATIONS = 20;
    constexpr float learning_rate = 0.001f;

    const GameBoard board{};
    NNUETrainingExample example{};
    NNUE_data nnue;

    nnue.fill_nnue_inputs_fast(board, example.inputs);

    // The original material score was zero, so the positional target is 75 cp.
    example.target_positional_score_cp = 75.0f;

    // Start with one active neuron, as in test_NNUE_training_one_position().
    NNUEParameters parameters{};
    parameters.Neurons[0].bias = 1.0f;
    parameters.output_weight[0] = 1.0f;
    nnue.set_parameters(parameters);

    NNUEGradient gradient{};
    ASSERT_FLOAT_EQ(nnue.Calculate_nnue_aberration(example), -74.0f);

    for (int iteration = 0; iteration < TRAINING_ITERATIONS; ++iteration) {
        SCOPED_TRACE(iteration);

        const float aberration = nnue.Calculate_nnue_aberration(example);

        // const float nnue_positional =
        //     example.target_positional_score_cp + aberration;
        // std::ostringstream aberration_text;
        // aberration_text << std::fixed << std::setprecision(10) << aberration;
        // sout << "Iteration: " << iteration
        //      << "  NNUE positional: " << nnue_positional
        //      << "  Aberration: " << aberration_text.str()
        //      << std::endl;

        nnue.calculate_nnue_gradient(example, gradient);
        nnue.apply_nnue_gradient(gradient, learning_rate);

        const float updated_aberration = nnue.Calculate_nnue_aberration(example);
        if (std::fabs(aberration) > FLT_EPSILON) {
            EXPECT_LT(std::fabs(updated_aberration), std::fabs(aberration));
        } else {
            // Once the error reaches zero, further updates must keep it there.
            EXPECT_NEAR(updated_aberration, 0.0f, FLT_EPSILON);
        }

        if (iteration == TRAINING_ITERATIONS - 1) {
            EXPECT_NEAR(updated_aberration, 0.0f, FLT_EPSILON);
        }
    }
}

} // namespace
