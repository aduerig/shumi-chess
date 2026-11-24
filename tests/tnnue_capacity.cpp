#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "NNUE.hpp"
#include "gameboard.hpp"

namespace {
using namespace ShumiChess;
namespace fs = std::filesystem;

void expect_zero_neurons(const NNUEParameters& parameters, int first)
{
    for (int j = first; j < NNUE_NEURON_COUNT; ++j) {
        SCOPED_TRACE(j);
        EXPECT_EQ(parameters.output_weight[j], 0.0f);
        EXPECT_EQ(parameters.Neurons[j].bias, 0.0f);
        EXPECT_EQ(parameters.Neurons[j].turn_weight, 0.0f);
        for (const auto& piece : parameters.Neurons[j].piece_weight)
            for (float weight : piece) EXPECT_EQ(weight, 0.0f);
    }
}

void expect_same_parameters(const NNUEParameters& a, const NNUEParameters& b)
{
    EXPECT_EQ(a.output_bias, b.output_bias);
    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
        SCOPED_TRACE(j);
        EXPECT_EQ(a.output_weight[j], b.output_weight[j]);
        EXPECT_EQ(a.Neurons[j].bias, b.Neurons[j].bias);
        EXPECT_EQ(a.Neurons[j].turn_weight, b.Neurons[j].turn_weight);
        for (int p = 0; p < 12; ++p)
            for (int s = 0; s < 64; ++s)
                EXPECT_EQ(a.Neurons[j].piece_weight[p][s], b.Neurons[j].piece_weight[p][s]);
    }
}

class NNUECapacity : public ::testing::Test {
protected:
    const fs::path repo = fs::path(__FILE__).parent_path().parent_path();
    fs::path directory, saved;
    std::vector<NNUETrainingExample> examples;
    std::vector<std::string> fens;

    void SetUp() override
    {
        directory = fs::temp_directory_path() / ("shumi_nnue_capacity_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        ASSERT_TRUE(fs::create_directory(directory));
        saved = directory / "saved.mod";
        std::string error;
        ASSERT_EQ(multi_FEN_readin(repo / "tests/FENs/five_trainings_positions.txt", examples,
            NNUETrainingMethod::NONE, 0, 0, UNCLE_SHUMI, false, false, &error, &fens), 0) << error;
    }

    void TearDown() override
    {
        for (const auto& path : {saved, directory}) {
            if (path.empty()) continue;
            std::error_code error;
            fs::remove(path, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    std::string read(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        EXPECT_TRUE(file.is_open());
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
};

TEST_F(NNUECapacity, Loading64StoredNeuronsClearsUpperSlotsAndPreservesMetadata)
{
    ASSERT_GE(NNUE_NEURON_COUNT, 64);
    const auto legacy = repo / "NNUE/models/63_344K_400_A.mod";
    ASSERT_NE(read(legacy).find("# Neuron count: 64\r\n"), std::string::npos);
    NNUE_data nnue;
    nnue.initialize_parameters_bounded(NNUE_NEURON_COUNT, {0.3f, 0.3f, 0.1f, 0.25f, 0, 0, 0.5f, 0});
    ASSERT_NE(nnue.get_parameters().Neurons[NNUE_NEURON_COUNT - 1].bias, 0.0f);
    ASSERT_TRUE(nnue.load_parameters(legacy));
    // A 64-neuron build has no extra slots; larger builds must clear them all.
    expect_zero_neurons(nnue.get_parameters(), 64);
    ASSERT_TRUE(nnue.save_parameters(saved));
    const auto text = read(saved);
    EXPECT_NE(text.find("# Format version: 1\r\n# Neuron count: " +
              std::to_string(NNUE_NEURON_COUNT) + "\r\n# Activated neurons: 63\r\n"),
              std::string::npos);
    NNUE_data reloaded;
    ASSERT_TRUE(reloaded.load_parameters(saved));
    expect_same_parameters(nnue.get_parameters(), reloaded.get_parameters());
    for (const auto& fen : fens) {
        GameBoard board(fen);
        EXPECT_EQ(nnue.Calculate_nnue_positional(board), reloaded.Calculate_nnue_positional(board));
    }
}

TEST_F(NNUECapacity, TrainsAndRoundTripsNetworks)
{
    constexpr float learning_rate = 0.0001f;
    constexpr int iterations = 3;
    for (int active : {1, 31, 64}) {
        SCOPED_TRACE(active);
        NNUE_data nnue;
        const auto result = NNUE_train_mult_positions(nnue, examples.data(),
            static_cast<int>(examples.size()), active, iterations, learning_rate, false);
        EXPECT_TRUE(std::isfinite(result.initial_rmse));
        EXPECT_TRUE(std::isfinite(result.final_rmse));
        expect_zero_neurons(nnue.get_parameters(), active);
        ASSERT_TRUE(nnue.save_parameters(saved, {}, active, iterations, learning_rate, result));
        const auto text = read(saved);
        EXPECT_NE(text.find("# Neuron count: " + std::to_string(NNUE_NEURON_COUNT) +
            "\r\n# Activated neurons: " + std::to_string(active) + "\r\n"), std::string::npos);
        NNUE_data reloaded;
        ASSERT_TRUE(reloaded.load_parameters(saved));
        expect_same_parameters(nnue.get_parameters(), reloaded.get_parameters());
        EXPECT_EQ(nnue.Calculate_nnue_RMSe(examples), reloaded.Calculate_nnue_RMSe(examples));
        for (const auto& fen : fens) {
            GameBoard board(fen);
            const float score = nnue.Calculate_nnue_positional(board);
            EXPECT_TRUE(std::isfinite(score));
            EXPECT_EQ(score, reloaded.Calculate_nnue_positional(board));
        }
    }
}

TEST_F(NNUECapacity, CopiesKeepIndependentParametersAndModelMetadata)
{
    NNUE_data original;
    ASSERT_TRUE(original.load_parameters(repo / "NNUE/models/63_344K_400_A.mod"));
    ASSERT_TRUE(original.save_parameters(saved));
    const auto expected_file = read(saved);
    NNUE_data copied(original);
    NNUE_data assigned;
    assigned = original;
    original.zero_parameters();
    EXPECT_NE(&copied.get_parameters(), &assigned.get_parameters());
    EXPECT_NE(&copied.get_parameters(), &original.get_parameters());
    expect_same_parameters(copied.get_parameters(), assigned.get_parameters());
    ASSERT_TRUE(copied.save_parameters(saved));
    EXPECT_EQ(read(saved), expected_file);
    ASSERT_TRUE(assigned.save_parameters(saved));
    EXPECT_EQ(read(saved), expected_file);
    const GameBoard board(fens[0]);
    EXPECT_EQ(original.Calculate_nnue_positional(board), 0.0f);
    EXPECT_EQ(copied.Calculate_nnue_positional(board), assigned.Calculate_nnue_positional(board));
    copied.zero_parameters();
    ASSERT_TRUE(assigned.save_parameters(saved));
    EXPECT_EQ(read(saved), expected_file);
}

} // namespace
