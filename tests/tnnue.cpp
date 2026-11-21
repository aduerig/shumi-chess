#include <gtest/gtest.h>

#include <cfloat>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

#include "NNUE.hpp"
#include "engine.hpp"
#include "gameboard.hpp"
#include "minimax.hpp"
#include "status_output.hpp"

namespace {

using namespace ShumiChess;

constexpr int square_e5 = GameBoard::square_e5;

class NNUEFENReader : public ::testing::Test {
protected:
    std::filesystem::path directory;
    std::filesystem::path filename;

    void SetUp() override
    {
        directory = std::filesystem::temp_directory_path() /
            ("shumi_nnue_reader_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        ASSERT_TRUE(std::filesystem::create_directory(directory));
        filename = directory / "positions.txt";
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove(filename, error);
        EXPECT_FALSE(error) << error.message();
        std::filesystem::remove(directory, error);
        EXPECT_FALSE(error) << error.message();
    }

    void write(const std::string& text)
    {
        std::ofstream file(filename);
        file << text;
        file.close();
        ASSERT_TRUE(file) << filename;
    }
};

TEST_F(NNUEFENReader, RejectsMalformedRecordsAndModeMismatches)
{
    const std::string fen = "7k/8/8/4N3/8/8/8/K7 w - - 0 1";
    const std::string bad_suffixes[] = {
        "", " |", " |  ", " | +", " | -", " | +-2", " | --2",
        " | 1.5", " | 1e2", " | 7 extra", " | * extra", " | 1 | 2",
        " | **", " | 2147483648", " | -2147483649", " | # comment"
    };
    // Every record below is rejected before evaluation or search can begin.
    for (const auto mode : {NNUETrainingMethod::NONE, NNUETrainingMethod::STATIC_EVAL,
                            NNUETrainingMethod::FULL_SEARCH}) {
        for (const auto& suffix : bad_suffixes) {
            SCOPED_TRACE(suffix);
            SCOPED_TRACE(static_cast<int>(mode));
            write("# header\n\n" + fen + suffix + "\n");
            std::vector<NNUETrainingExample> examples(1);
            examples[0].target_positional_score_cp = 123.0f;
            std::string error;
            EXPECT_NE(multi_FEN_readin(filename, examples, mode, 0, 0, UNCLE_SHUMI,
                                     false, false, &error), 0);
            EXPECT_NE(error.find(filename.string()), std::string::npos);
            EXPECT_NE(error.find("line 3"), std::string::npos);
            ASSERT_EQ(examples.size(), 1u);
            EXPECT_FLOAT_EQ(examples[0].target_positional_score_cp, 123.0f);
        }
        write(fen + (mode == NNUETrainingMethod::NONE ? " | *\n" : " | 41\n"));
        std::vector<NNUETrainingExample> examples;
        std::string error;
        EXPECT_NE(multi_FEN_readin(filename, examples, mode, 0, 0, UNCLE_SHUMI,
                                 false, false, &error), 0);
        EXPECT_FALSE(error.empty());
        EXPECT_TRUE(examples.empty());
    }
}


TEST_F(NNUEFENReader, FullSearchSkipsMatesAndKeepsFensAligned)
{
    // The extra rook makes these mate targets fall below the mate threshold
    // after material subtraction. They must be filtered using the raw score.
    const std::string white_mate_in_one = "7k/5Q2/6K1/8/8/8/8/R7 w - - 0 1";
    const std::string black_mate_in_one = "r7/8/8/8/8/6k1/5q2/7K b - - 0 1";
    const std::string black_checkmated = "k7/1Q6/2K5/8/8/8/8/R7 b - - 0 1";
    const std::string white_checkmated = "r7/8/8/8/8/2k5/1q6/K7 w - - 0 1";
    const std::string kings_only = "7k/8/8/8/8/8/8/K7 w - - 17 29";
    const std::string stalemate = "7k/5Q2/6K1/8/8/8/8/8 b - - 0 42";
    write("# Interleave retained positions and mates.\n" +
          white_mate_in_one + " | *\n" +
          kings_only + " | *\n" +
          black_checkmated + " | *\n" +
          stalemate + " | *\n" +
          black_mate_in_one + " | *\n" +
          white_checkmated + " | *\n");

    std::vector<NNUETrainingExample> examples;
    std::vector<std::string> retained_fens{"old FEN"};
    std::string error;
    ASSERT_EQ(multi_FEN_readin(filename, examples, NNUETrainingMethod::FULL_SEARCH,
                              1, 1, UNCLE_SHUMI, false, false, &error, &retained_fens), 0) << error;
    EXPECT_TRUE(error.empty());
    ASSERT_EQ(examples.size(), 2u);
    ASSERT_EQ(retained_fens.size(), examples.size());
    EXPECT_EQ(retained_fens[0], kings_only);
    EXPECT_EQ(retained_fens[1], stalemate);
    EXPECT_EQ(examples[0].inputs.black_to_move, 0);
    EXPECT_EQ(examples[1].inputs.black_to_move, 1);
    EXPECT_FLOAT_EQ(examples[0].target_positional_score_cp, 0.0f);
    // Draw score zero minus White's queen material; this is a valid target.
    EXPECT_FLOAT_EQ(examples[1].target_positional_score_cp, -900.0f);
}

TEST_F(NNUEFENReader, AllMateSearchFailsWithoutReplacingOutputs)
{
    write("k7/1Q6/2K5/8/8/8/8/R7 b - - 0 1 | *\n");
    std::vector<NNUETrainingExample> examples(1);
    examples[0].target_positional_score_cp = 123.0f;
    std::vector<std::string> retained_fens{"old FEN"};
    std::string error;
    EXPECT_NE(multi_FEN_readin(filename, examples, NNUETrainingMethod::FULL_SEARCH,
                              1, 1, UNCLE_SHUMI, false, false, &error, &retained_fens), 0);
    EXPECT_NE(error.find("no usable FEN positions"), std::string::npos);
    ASSERT_EQ(examples.size(), 1u);
    EXPECT_FLOAT_EQ(examples[0].target_positional_score_cp, 123.0f);
    ASSERT_EQ(retained_fens.size(), 1u);
    EXPECT_EQ(retained_fens[0], "old FEN");
}


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

        nnue.train_one_position(example, learning_rate);

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
// Use the fixture's integer targets with NONE and initialize one active neuron.
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
        std::filesystem::path(__FILE__).parent_path() / "FENs" / "five_trainings_positions.txt";

    std::string database_error;
    const int read_error = multi_FEN_readin(
        database_path, examples, NNUETrainingMethod::NONE, 0, 0, UNCLE_SHUMI
        , false, false, &database_error);
        
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

            nnue.train_one_position(example, learning_rate);
            
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
        database_path, examples, NNUETrainingMethod::NONE, 0, 0, UNCLE_SHUMI,
        false, false, &database_error);
    ASSERT_EQ(read_error, 0) << database_error;
    ASSERT_FALSE(examples.empty());

    const int position_count = static_cast<int>(examples.size());
    double starting_final_rmse = 0.0;
    int starting_nlocks = 0;

    for (int nActiveNeurons = min_neurons; nActiveNeurons <= max_neurons; ++nActiveNeurons) {
        SCOPED_TRACE(::testing::Message() << "nActiveNeurons=" << nActiveNeurons);

        // Start a fresh network; the existing training helper hard-codes 12.
        NNUE_data nnue;
        nnue.initialize_parameters_bounded(nActiveNeurons, bounds);

        const double initial_rmse = nnue.Calculate_nnue_RMSe(examples);
        //sout << "nActiveNeurons=" << nActiveNeurons << std::endl;
        //sout << "Initial RMSe: " << initial_rmse << " cp" << std::endl;
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
        //sout << "Final RMSe=" << final_rmse << " nlocks=" << nlocks << std::endl;

        EXPECT_TRUE(std::isfinite(final_rmse));
        EXPECT_LE(final_rmse, initial_rmse);
        if (nActiveNeurons == min_neurons) {
            starting_final_rmse = final_rmse;
            starting_nlocks = nlocks;
        } else if (nActiveNeurons == max_neurons) {
            EXPECT_LE(final_rmse, starting_final_rmse)
                << "Final RMSe increased from " << min_neurons << " to " << max_neurons << " neurons";
            EXPECT_LE(nlocks, starting_nlocks)
                << "nlocks increased from " << min_neurons << " to " << max_neurons << " neurons";
        }
    }
}

class NNUEModelFormat : public ::testing::Test {
protected:
    const std::filesystem::path model_path =
        std::filesystem::path(__FILE__).parent_path() / "data" / "Model_001.txt";
    std::filesystem::path directory, input_path, saved_path;
    std::string original, without_footer, parameter_text, rmse_line;
    std::size_t metadata_end = 0;

    std::string read(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        EXPECT_TRUE(file.is_open()) << path;
        std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        EXPECT_FALSE(file.bad()) << path;
        return text;
    }

    void write(const std::string& text)
    {
        std::ofstream file(input_path, std::ios::binary);
        file << text;
        file.close();
        ASSERT_TRUE(file) << input_path;
    }

    void SetUp() override
    {
        directory = std::filesystem::temp_directory_path() /
            ("shumi_nnue_model_format_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        ASSERT_TRUE(std::filesystem::create_directory(directory)) << directory;
        input_path = directory / "input.txt";
        saved_path = directory / "saved.txt";
        original = read(model_path);
        const auto footer = original.rfind("#\r\n# RMSe (cp): initial = ");
        ASSERT_NE(footer, std::string::npos);
        without_footer = original.substr(0, footer);
        rmse_line = original.substr(footer + 3);
        const auto learning_rate = original.find("# Learning rate: ");
        ASSERT_NE(learning_rate, std::string::npos);
        metadata_end = original.find("\r\n", learning_rate) + 2;
        const auto first_neuron = without_footer.find("# Neuron: 0\r\n");
        ASSERT_NE(first_neuron, std::string::npos);
        parameter_text = without_footer.substr(first_neuron);
    }

    void TearDown() override
    {
        for (const auto& path : {input_path, saved_path, directory}) {
            if (path.empty()) continue;
            std::error_code error;
            std::filesystem::remove(path, error);
            EXPECT_FALSE(error) << "Could not remove " << path << ": " << error.message();
        }
    }
};

TEST_F(NNUEModelFormat, MovesFooterIntoHeaderWithoutChangingOtherBytes)
{
    NNUE_data nnue;
    ASSERT_TRUE(nnue.load_parameters(model_path));
    ASSERT_TRUE(nnue.save_parameters(saved_path));
    std::string expected = without_footer;
    expected.insert(metadata_end, rmse_line);
    EXPECT_EQ(read(saved_path), expected);

    // Once in the new layout, load/save preserves every byte, including CRLF.
    NNUE_data reloaded;
    ASSERT_TRUE(reloaded.load_parameters(saved_path));
    ASSERT_TRUE(reloaded.save_parameters(input_path));
    EXPECT_EQ(read(input_path), expected);
}

TEST_F(NNUEModelFormat, WritesSuppliedTrainingResultsDirectlyAfterLearningRate)
{
    NNUE_data nnue;
    ASSERT_TRUE(nnue.load_parameters(model_path));
    const NNUETrainingResult result{98.17, 19.39, 0.0};
    ASSERT_TRUE(nnue.save_parameters(saved_path, {}, 28, 200, 0.0001f, result));
    const std::string saved = read(saved_path);
    const std::string expected = "# Learning rate: 1e-04\r\n"
        "# RMSe (cp): initial = 98.17, final = 19.39, lck% = 0.00\r\n#\r\n# Neuron: 0\r\n";
    EXPECT_NE(saved.find(expected), std::string::npos);
    const auto first_neuron = saved.find("# Neuron: 0\r\n");
    ASSERT_NE(first_neuron, std::string::npos);
    EXPECT_EQ(saved.substr(first_neuron), parameter_text)
        << "Moving the two-decimal RMSe display must not round model weights";
    NNUE_data reloaded;
    ASSERT_TRUE(reloaded.load_parameters(saved_path));
    ASSERT_TRUE(reloaded.save_parameters(input_path));
    EXPECT_EQ(read(input_path), saved);
}

TEST_F(NNUEModelFormat, LoadsLegacyFootersHeadersAndModelsWithoutTrainingResults)
{
    const std::string unavailable = "# RMSe (cp): initial = unavailable, final = unavailable, lck% = unavailable\r\n";
    const std::string old_rmse = "# RMSe (cp): initial = 123.45, final = 6.78\r\n";
    const std::string old_resaved = "# RMSe (cp): initial = 123.45, final = 6.78, lck% = unavailable\r\n";
    const std::string full_rmse = "# RMSe (cp): initial = 123.45, final = 6.78, lck% = 12.34\r\n";
    std::string old_header = without_footer, full_header = without_footer;
    old_header.insert(metadata_end, old_rmse);
    full_header.insert(metadata_end, full_rmse);
    const std::string minimal = "# Shumi NNUE parameters\r\n# Format version: 1\r\n# Neuron count: "
        + std::to_string(NNUE_NEURON_COUNT) + "\r\n" + parameter_text;
    const std::pair<std::string, std::string> cases[] = {
        {original, unavailable},
        {without_footer, unavailable},
        {without_footer + "#\r\n" + old_rmse, old_resaved},
        {without_footer + "#\r\n" + full_rmse, full_rmse},
        {old_header, old_resaved},
        {full_header, full_rmse},
        {minimal, unavailable},
    };
    for (std::size_t i = 0; i < std::size(cases); ++i) {
        SCOPED_TRACE(i);
        write(cases[i].first);
        NNUE_data nnue;
        ASSERT_TRUE(nnue.load_parameters(input_path));
        ASSERT_TRUE(nnue.save_parameters(saved_path));
        const std::string saved = read(saved_path);
        EXPECT_NE(saved.find(cases[i].second + "#\r\n# Neuron: 0\r\n"), std::string::npos);
        const auto first_neuron = saved.find("# Neuron: 0\r\n");
        ASSERT_NE(first_neuron, std::string::npos);
        EXPECT_EQ(saved.substr(first_neuron), parameter_text);
    }
}

} // namespace
