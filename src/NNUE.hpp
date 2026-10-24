#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "globals.hpp"


class MinimaxAI;  // Before namespace ShumiChess

namespace ShumiChess {

class GameBoard;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Yes—the inputs can describe the chess “assembly”: the pieces arranged on the board. That explains why the optical analogy was confusing.
// The key difference is what we are optimizing:
// - In optical design, you optimize the physical assembly, changing thicknesses and separations.
// - Here, we hold the chess positions fixed and optimize the scoring formula, changing its coefficients.
// So NNUE parameter values belong to the scoring machinery. They are not board properties. Calling them score-calculation coefficients is a useful way to think about them, though intermediate coefficients are not necessarily centipawn scores.
// The aberration remains separate: calculated score minus target score.


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct NNUEInputs {
    // White: pawn, knight, bishop, rook, queen, king.
    // Black: pawn, knight, bishop, rook, queen, king.
    uint8_t piece_on_square[12][64];
    uint8_t black_to_move;
};

struct NNUENeuronParameters {
    float piece_weight[12][64]; // 768 adjustable numbers
    float turn_weight;         // One adjustable number
    float bias;                // One adjustable number
};

#define NNUE_NEURON_COUNT 32

struct NNUEParameters {
    NNUENeuronParameters Neurons[NNUE_NEURON_COUNT];

    float output_weight[NNUE_NEURON_COUNT];
    float output_bias;
};

// One fixed training case.
// All scores are in centipawns, positive good for White.
// These are training data, not adjustable network parameters.
// struct NNUETrainingExample {
//     NNUEInputs inputs;
//     float material_score_cp;
//     float target_search_score_cp;
// };
struct NNUETrainingExample {
    NNUEInputs inputs;
    float target_positional_score_cp;
};

// One Jacobian row: derivatives of ONE example's aberration
// with respect to every network parameter.
// Same layout as NNUEParameters, but contains sensitivities,
// not parameter values.
using NNUEJacobianRow = NNUEParameters;

// So we multiply every entry of your Jacobian row by the aberration.
// Derivatives of the merit function with respect to all parameters.
// Same layout as the parameters, but stores the gradient.
using NNUEGradient = NNUEParameters;

// Add one example's gradient to the accumulated gradient.
// This adds sensitivities of the merit function, not parameter values.
// No network parameter values are changed.
// void add_nnue_gradient(
//     NNUEGradient& total,
//     const NNUEGradient& contribution);

///////////////////////////////////////////////////////////////////////////////////////////////////

// Owns one network's scoring parameters. Keep one instance with the evaluator
// and reuse it for successive positions. Board inputs are prepared per call;
// the network does not retain a board or a reference to one.
class NNUE_data {

public:
    // Every weight and bias starts at zero, so the positional score is zero.
    // This is useful for the initial assertion check. Training needs nonzero
    // starting parameters, supplied through the other constructor or setter.
    NNUE_data();
    explicit NNUE_data(const NNUEParameters& initial_parameters);

    int n_calculates = 0; // Public heartbeat: completed positional-score calculations.

    // Parameters are owned by value; supplying them copies their contents.
    const NNUEParameters& get_parameters() const;
    void set_parameters(const NNUEParameters& parameters);
    void zero_parameters();

    void fill_nnue_inputs(const GameBoard& board, NNUEInputs& inputs) const;
    void fill_nnue_inputs_fast(const GameBoard& board, NNUEInputs& inputs) const;

    // Whole-position centipawn score, positive good for White.
    // Both overloads leave the stored parameters unchanged.
    float Calculate_nnue_positional(const GameBoard& board);

    void initialize_parameters();

    // Training calculations use the same stored parameters as evaluation.
    // Examples, Jacobian rows, and gradients belong to the training caller.
    float Calculate_nnue_aberration(const NNUETrainingExample& example);

    void calculate_nnue_jacobian_row(
        const NNUEInputs& inputs,
        NNUEJacobianRow& row) const;
        
    // called ony in testing
    float calculate_nnue_gradient(
        const NNUETrainingExample& example,
        NNUEGradient& gradient) const;

    // Called only in testing. This changes the stored parameters; calculate the gradient first.
    void apply_nnue_gradient(const NNUEGradient& gradient, float learning_rate);

private:
    static constexpr std::size_t NNUE_INPUT_COUNT = 12 * 64;

    // Inference stores all neurons for one piece-square feature together.
    // The public parameter/Jacobian layout remains unchanged for training.
    struct alignas(64) NNUEInferenceParameters {

        float piece_weight[12][64][NNUE_NEURON_COUNT];

        float bias[NNUE_NEURON_COUNT];
        float turn_weight[NNUE_NEURON_COUNT];
        float output_weight[NNUE_NEURON_COUNT];
        float output_bias;
    };

    void rebuild_inference_parameters_();

    struct NNUEActiveInput {
        std::uint16_t feature_index;
        std::uint8_t value;
    };

    static std::size_t collect_active_inputs(
        const NNUEInputs& inputs,
        NNUEActiveInput (&active_inputs)[NNUE_INPUT_COUNT]);

    static std::size_t collect_active_inputs_from_bitboards(
            const ull (&piece_bitboards)[12],
            NNUEActiveInput* active_inputs);


    static float calculate_neuron(
        const NNUEInputs& inputs,
        const NNUENeuronParameters& parameters,
        const NNUEActiveInput* active_inputs,
        std::size_t active_count);

    static inline float activate_unit(float value)
    {
        return (value > 0.0f) ? value : 0.0f;
    }


    static float calculate_nnue_positional_training(
        const NNUEInputs& inputs,
        const NNUEParameters& parameters);

    static float calculate_nnue_positional_(
        std::uint8_t black_to_move,
        const NNUEInferenceParameters& parameters,
        const NNUEActiveInput* active_inputs,
        std::size_t active_count);

    // Let training reuse the private scorer with caller-supplied parameters.
    // friend float Calculate_nnue_aberration(
    //     const NNUETrainingExample& example,
    //     const NNUEParameters& parameters);

    // Calculate the training aberration for one position:
    // aberration = material + network positional score - target search score.
    //
    // Analogous to calculated optical performance minus desired performance.
    // Does not change any parameters.
    float calculate_nnue_aberration_(
        const NNUETrainingExample& example,
        const NNUEParameters& parameters);



    // Calculate one Jacobian row analytically.
    // Material and target scores are fixed, so their derivatives are zero.
    // Therefore, aberration derivatives equal network-output derivatives.
    // Does not change the network parameters.
    static void calculate_nnue_jacobian_row_(
        const NNUEInputs& inputs,
        const NNUEParameters& parameters,
        NNUEJacobianRow& row);

    // Calculate one training example's gradient contribution.
    // Merit function: L = 0.5 * aberration * aberration.
    // Gradient: g[j] = aberration * Jacobian_row[j].
    // Does not change the network parameters.
    // Returns the aberration for inspection.
    static float _calculate_nnue_gradient(
        const NNUETrainingExample& example,
        const NNUEParameters& parameters,
        NNUEGradient& gradient);

    // Apply a gradient-descent parameter adjustment:
    // new_parameter = old_parameter - learning_rate * gradient.
    // The gradient must have been calculated before any parameters change.
    static void apply_nnue_gradient_(
        NNUEParameters& parameters,
        const NNUEGradient& gradient,
        float learning_rate);

    NNUEParameters parameters_;

    // Keep the large inference copy off the stack. Copies can safely share it;
    // changing parameters replaces the cache instead of modifying it in place.
    std::shared_ptr<const NNUEInferenceParameters> inference_parameters_;

    

};

void test_NNUE1();
void test_NNUE_training_one_position(MinimaxAI*);

} // end namespace ShumiChess
