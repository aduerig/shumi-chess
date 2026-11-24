#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

#include "NNUE.hpp"
#include "status_output.hpp"

#ifdef _WIN32
    #include <conio.h>
#else
    #include <iostream>
    static inline int _getch() { return std::cin.get(); }
#endif

namespace {

using namespace ShumiChess;
namespace fs = std::filesystem;

constexpr const char* default_validation_file = "NNUE/FENs/evaluated_FENs.txt";   // NNUE\FENs\evaluated_FENs.txt
constexpr const char* default_model_directory = "NNUE/models";

struct NeuronCounts {
    int stored = 0;
    int active = -1;
};

// Call only after load_parameters has validated the entire model. The loader
// keeps this metadata private; read just these two header fields for reporting
// and the inactive-slot check. Parameter parsing remains entirely in the loader.
NeuronCounts read_neuron_counts(const fs::path& filename)
{
    std::ifstream file(filename);
    NeuronCounts counts;
    std::string line;
    const std::string stored_label = "# Neuron count: ";
    const std::string active_label = "# Activated neurons: ";
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "# Neuron: 0") break;
        if (line.rfind(stored_label, 0) == 0) {
            counts.stored = std::stoi(line.substr(stored_label.size()));
        } else if (line.rfind(active_label, 0) == 0) {
            counts.active = std::stoi(line.substr(active_label.size()));
        }
    }
    if (!file || counts.stored <= 0 || counts.stored > NNUE_NEURON_COUNT ||
        counts.active < -1 || counts.active > counts.stored) {
        throw std::runtime_error("could not read validated neuron metadata");
    }
    return counts;
}

// Shumi training leaves unused neuron slots zero; the existing scorer sums all
// compiled slots. Verify that this is equivalent to using the declared active
// count, without altering any saved parameters or the inference mathematics.
bool inactive_slots_are_zero(const NNUEParameters& parameters, int active)
{
    for (int j = active; j < NNUE_NEURON_COUNT; ++j) {
        const auto& neuron = parameters.Neurons[j];
        if (parameters.output_weight[j] != 0.0f ||
            neuron.bias != 0.0f || neuron.turn_weight != 0.0f) return false;
        for (const auto& piece : neuron.piece_weight) {
            for (float weight : piece) {
                if (weight != 0.0f) return false;
            }
        }
    }
    return true;
}

std::vector<NNUETrainingExample> read_examples(const fs::path& filename)
{
    std::vector<NNUETrainingExample> examples;
    std::string error;
    // NONE uses the stored absolute positional targets directly. It performs no
    // evaluation, search, material subtraction, randomization, or filtering.
    if (multi_FEN_readin(filename, examples, NNUETrainingMethod::NONE,
                        0, 0, UNCLE_SHUMI, false, false, &error) != 0) {
        throw std::runtime_error(error);
    }
    return examples;
}

std::vector<fs::path> discover_models(const fs::path& directory)
{
    std::vector<fs::path> models;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".mod" && entry.is_regular_file()) {
            models.push_back(entry.path());
        }
    }
    std::sort(models.begin(), models.end(), [](const auto& left, const auto& right) {
        return left.filename().native() < right.filename().native();
    });
    if (models.empty()) {
        throw std::runtime_error("no regular .mod files found in " + directory.string());
    }
    return models;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc > 3 || (argc > 1 && std::string(argv[1]) == "--help")) {
        sout << "Usage: validate_models [validation_file [model_directory]]\n"
             << "Defaults: " << default_validation_file << " and "
             << default_model_directory << "\n"
             << "Relative paths are resolved from the repository root.\n"
             << "Exit status is nonzero if an input or any model fails.\n";
        return argc > 3 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    const auto started_at = std::chrono::steady_clock::now();
    std::size_t succeeded = 0;
    std::size_t failed = 0;
    bool input_failed = false;
    bool used_stored_count = false;
    sout << "ShumiNNUE Model Validation\n==========================\n\n";

    try {
        // Follow the training driver's repository-relative path convention.
        const fs::path repo_root = fs::path(__FILE__).parent_path().parent_path();
        const fs::path input_path = repo_root /
            (argc > 1 ? argv[1] : default_validation_file);
        const fs::path model_directory = repo_root /
            (argc > 2 ? argv[2] : default_model_directory);
        sout << "Validation file:\n" << input_path.string()
             << "\n\nModel directory:\n" << model_directory.string() << "\n\n";

        // Load and encode once. Every model receives this same immutable vector
        // in the original record order, with the original positional targets.
        const auto examples = read_examples(input_path);
        sout << "Validation positions: " << examples.size() << "\n\n";
        const auto models = discover_models(model_directory);
        std::size_t name_width = 29;
        for (const auto& path : models) {
            name_width = std::max(name_width, path.filename().string().size() + 2);
        }
        sout << std::left << std::setw(static_cast<int>(name_width)) << "Model"
             << std::right << std::setw(10) << "Neurons" << std::setw(14) << "RMSE (cp)"
             << '\n' << std::string(name_width + 24, '-') << '\n';

        for (const auto& path : models) {
            try {
                NNUE_data nnue;
                if (!nnue.load_parameters(path)) {
                    // The loader supplies the filename, line, and precise reason.
                    throw std::runtime_error("model load failed; see diagnostic above");
                }
                const auto counts = read_neuron_counts(path);
                // Legacy models without active metadata use all stored neurons.
                const int active = counts.active >= 0 ? counts.active : counts.stored;
                if (!inactive_slots_are_zero(nnue.get_parameters(), active)) {
                    throw std::runtime_error("nonzero parameters beyond the declared active-neuron count");
                }
                const double rmse = nnue.Calculate_nnue_RMSe(examples);
                if (!std::isfinite(rmse)) {
                    throw std::runtime_error("non-finite validation RMSE (numerical overflow)");
                }
                const std::string neurons = std::to_string(active) +
                    (counts.active < 0 ? "*" : "");
                sout << std::left << std::setw(static_cast<int>(name_width))
                     << path.filename().string() << std::right << std::setw(10) << neurons
                     << std::setw(14) << std::fixed << std::setprecision(4) << rmse << '\n';
                used_stored_count = used_stored_count || counts.active < 0;
                ++succeeded;
            } catch (const std::exception& error) {
                ++failed;
                sout << path.filename().string() << ": FAILED - " << error.what() << '\n';
            }
        }
    } catch (const std::exception& error) {
        input_failed = true;
        sout << "ERROR: " << error.what() << '\n';
    }

    if (used_stored_count) {
        sout << "* Active count absent from metadata; using all stored neurons.\n";
    }
    const double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started_at).count();
    sout << (input_failed ? "\nValidation stopped. " : "\nValidation complete. ")
         << "Models: " << succeeded << " succeeded, " << failed << " failed.\n"
         << "Total elapsed time: " << std::fixed << std::setprecision(3) << seconds << " s\n";
    sout << "Press any key to exit..." << std::endl;
    _getch();
    return input_failed || failed != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
