#include <gtest/gtest.h>

#include <cfloat>
#include <cmath>

#include "NNUE.hpp"
#include "engine.hpp"
#include "gameboard.hpp"
#include "minimax.hpp"
#include "status_output.hpp"

namespace {

using namespace ShumiChess;

constexpr int square_e5 = GameBoard::square_e5;


// Verify that construction copies the supplied parameters into NNUE-owned storage.
// Changing the caller's weights afterward must not affect network predictions.
// Use a single neuron with known weights to check exact positional scores.
// Vary piece square, piece color, and side to move to check refreshed board inputs.
// Revisit the first position to ensure earlier evaluations leave no stale inputs.
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

// Verify that set_parameters installs an independent copy of the supplied network.
// Exercise the last neuron, piece channel, and square with a known expected score.
// Changing the caller's output bias must not change the installed parameters.
// Reset the network with zero_parameters and require an exactly zero prediction.
// Inspect representative stored weights and biases to confirm they were cleared.
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

// Verify training calculations and updates using a single active neuron.
// Build board inputs and a positional target from search score minus material.
// Check exact Jacobian entries, prediction error, and corresponding gradients.
// Apply one gradient step and require a smaller absolute prediction error.
// Confirm the owned weight changes while the caller's original parameters do not.
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

// With all NNUE parameters zero, evaluation using NNUE must equal the normal SLUG evaluation.
// NNUE scores are positive when good for White.
// Check that each positional score is added exactly once to the SLUG baseline.
// Exercise both score signs, both evaluator perspectives, and changing board inputs.
// Verify that scores of +1.5 and -1.5 centipawns round away from zero.
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

// Verify that repeated gradient updates fit one position to a 75-centipawn target.
// Initialize one active neuron so the first prediction is exactly 1 centipawn.
// Train for 20 iterations using the fast board inputs and a fixed learning rate.
// Require each step to reduce absolute error until it reaches float epsilon.
// Check that later steps preserve convergence and the final error is near zero.
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

// Still one neuron though
// Verify that one shared network learns all positions loaded from the FEN fixture.
// Build training targets with STATIC_EVAL and initialize one active neuron.
// Train each position over 20 passes, requiring finite errors after every update.
// Require final RMSE to be at most 0.01 centipawns across the loaded examples.
// Recheck every final error for that tolerance and improvement where applicable.
TEST(NNUEData, TrainingFivePositionsConverges)
{
    constexpr int TRAINING_ITERATIONS = 20;
    constexpr float learning_rate = 0.001f;
    constexpr float convergence_tolerance = 0.01f;

    // Get database of training examples
    std::vector<NNUETrainingExample> examples;

    const std::filesystem::path database_path =
        std::filesystem::path(__FILE__).parent_path() / "FENs" / "six_trainings_positions.txt";

    std::string database_error;
    const int read_error = multi_FEN_readin(
        database_path, examples, NNUETrainingMethod::STATIC_EVAL, &database_error);
    ASSERT_EQ(read_error, 0) << database_error;
    ASSERT_FALSE(examples.empty());

    // Initialize once, then train the same network on every loaded position.
    NNUE_data nnue;

     // Local storage for one full set of starting weights and biases; {} zeros them all.
    // The nnue object already owns separate working storage.
    NNUEParameters parameters{};

    // One neuron ONLY!
    // Neuron 0: z[0] = bias + sum(piece_weight[p][s] * piece_on_square[p][s])
    //                     + turn_weight * black_to_move. Its activation is max(0, z[0]).
    // All its input weights start at zero, so bias = 1 gives activation = max(0, 1) = 1.
    parameters.Neurons[0].bias = 20.0f;

    // Final score = output_bias + sum over neurons j of (output_weight[j] * activation[j]).
    // This weight scales neuron 0's activation; initially the score is 0 + 1 * 1 = 1 cp.
    // All other neurons contribute zero because their parameters are still zero.
    parameters.output_weight[0] = 1.0f;

    // Copy these starting values into nnue's own parameters and rebuild its fast-evaluation copy.
    // Later training updates nnue's copy; this local parameters object stays unchanged.
    nnue.set_parameters(parameters);



    const double initial_rmse = nnue.Calculate_nnue_RMSe(examples);
    //sout << "Initial RMSe: " << initial_rmse << " cp" << std::endl;

    std::vector<float> initial_aberrations;
    for (const auto& example : examples) {
        initial_aberrations.push_back(nnue.Calculate_nnue_aberration(example));
    }
    NNUEGradient gradient{};

    // Train the same network on every loaded position.
    for (int iteration = 0; iteration < TRAINING_ITERATIONS; ++iteration) {
        SCOPED_TRACE(iteration);

        for (std::size_t position = 0; position < examples.size(); ++position) {
            SCOPED_TRACE(position);
            const NNUETrainingExample& example = examples[position];

            // Printout
            // if (iteration == 0 || iteration == TRAINING_ITERATIONS - 1) {
            //     const float aberration = nnue.Calculate_nnue_aberration(example);
            //     std::ostringstream aberration_text;
            //     aberration_text << std::showpos << std::fixed << std::setprecision(5)
            //                     << aberration;

            //     sout << std::setw(4) << iteration
            //          << "  Pos: " << position
            //          << "  Abrr:" << aberration_text.str()
            //          << "  JJt  " << nnue.calculate_JJt(example.inputs)
            //          << std::endl;
            // }

            // Apply grdient
            nnue.calculate_nnue_gradient(example, gradient);
            nnue.apply_nnue_gradient(gradient, learning_rate);
            
            ASSERT_TRUE(std::isfinite(nnue.Calculate_nnue_aberration(example)));
      
        }

    }

    const double final_rmse = nnue.Calculate_nnue_RMSe(examples);
    //sout << "Final RMSe: " << final_rmse << " cp" << std::endl;
    EXPECT_LE(final_rmse, convergence_tolerance);

    // Later positions can change earlier predictions, so recheck all final errors.
    for (std::size_t position = 0; position < examples.size(); ++position) {
        SCOPED_TRACE(position);
        const float final_aberration = nnue.Calculate_nnue_aberration(examples[position]);
        if (std::fabs(initial_aberrations[position]) > convergence_tolerance) {
            EXPECT_LT(std::fabs(final_aberration), std::fabs(initial_aberrations[position]));
        }

        EXPECT_NEAR(final_aberration, 0.0f, convergence_tolerance);
    }
}

// Sweep the driver's training data with 3 through 16 initialized neurons.
// Keep the same bounds, learning rate, position order, and 100 training passes.
// Check each run against its initial RMSE, then compare the 3- and 16-neuron results.
// Allow ties, including when the number of locked positions reaches zero.
TEST(NNUEData, TrainingNeuronSweepImprovesRmseAndLocks)
{
    constexpr int min_neurons = 3;
    constexpr int max_neurons = 16;
    constexpr int TRAINING_ITERATIONS = 100;
    constexpr float learning_rate = 0.001f;
    const NNUEInitializationBounds bounds{0.3f, 0.3f, 0.0f, 0.25f};
    ASSERT_LE(max_neurons, NNUE_NEURON_COUNT);

    const std::filesystem::path database_path =
        std::filesystem::path(__FILE__).parent_path() / "FENs" / "fullGamr001_scrambled.txt";
    std::vector<NNUETrainingExample> examples;
    std::string database_error;
    const int read_error = multi_FEN_readin(
        database_path, examples, NNUETrainingMethod::STATIC_EVAL, &database_error);
    ASSERT_EQ(read_error, 0) << database_error;
    ASSERT_FALSE(examples.empty());

    const int position_count = static_cast<int>(examples.size());
    double starting_final_rmse = 0.0;
    int starting_nlocks = 0;

    for (int nNeurons = min_neurons; nNeurons <= max_neurons; ++nNeurons) {
        SCOPED_TRACE(::testing::Message() << "nNeurons=" << nNeurons);

        // Start a fresh network; the existing training helper hard-codes 12.
        NNUE_data nnue;
        nnue.initialize_parameters_bounded(nNeurons, bounds);

        const double initial_rmse = nnue.Calculate_nnue_RMSe(examples);
        sout << "nNeurons=" << nNeurons << std::endl;
        sout << "Initial RMSe: " << initial_rmse << " cp" << std::endl;
        EXPECT_TRUE(std::isfinite(initial_rmse));

        NNUEGradient gradient{};
        for (int iteration = 0; iteration < TRAINING_ITERATIONS; ++iteration) {
            for (const auto& example : examples) {
                nnue.calculate_nnue_gradient(example, gradient);
                nnue.apply_nnue_gradient(gradient, learning_rate);
            }
        }

        const double final_rmse = nnue.Calculate_nnue_RMSe(examples);
        const int nlocks = nnue.count_locked_positions(examples.data(), position_count);
        sout << "Final RMSe=" << final_rmse << " nlocks=" << nlocks << std::endl;

        EXPECT_TRUE(std::isfinite(final_rmse));
        EXPECT_LE(final_rmse, initial_rmse);
        if (nNeurons == min_neurons) {
            starting_final_rmse = final_rmse;
            starting_nlocks = nlocks;
        } else if (nNeurons == max_neurons) {
            EXPECT_LE(final_rmse, starting_final_rmse)
                << "Final RMSe increased from " << min_neurons << " to " << max_neurons << " neurons";
            EXPECT_LE(nlocks, starting_nlocks)
                << "nlocks increased from " << min_neurons << " to " << max_neurons << " neurons";
        }
    }
}

} // namespace
