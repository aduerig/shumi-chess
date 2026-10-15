#pragma once

#include <cstdint>

namespace ShumiChess {

class GameBoard;


// Yes—the inputs can describe the chess “assembly”: the pieces arranged on the board. That explains why the optical analogy was confusing.
// The key difference is what we are optimizing:
// - In optical design, you optimize the physical assembly, changing thicknesses and separations.
// - Here, we hold the chess positions fixed and optimize the scoring formula, changing its coefficients.
// So NNUE parameter values belong to the scoring machinery. They are not board properties. Calling them score-calculation coefficients is a useful way to think about them, though intermediate coefficients are not necessarily centipawn scores.
// The aberration remains separate: calculated score minus target score.





struct NNUEInputs {
    // White: pawn, knight, bishop, rook, queen, king.
    // Black: pawn, knight, bishop, rook, queen, king.
    uint8_t piece_on_square[12][64];
    uint8_t black_to_move;
};

struct NNUEUnitParameters {
    float piece_weight[12][64]; // 768 adjustable numbers
    float turn_weight;         // One adjustable number
    float bias;                // One adjustable number
};

#define NNUE_UNIT_COUNT 32

struct NNUEParameters {
    NNUEUnitParameters units[NNUE_UNIT_COUNT];

    float output_weight[NNUE_UNIT_COUNT];
    float output_bias;
};

void fill_nnue_inputs(const GameBoard& board, NNUEInputs& inputs);

// positional_score = output_bias
//                  + sum over j = 0..31 of:
//                    output_weight[j] * max(0, unit_value[j])

float calculate_nnue_positional(
    const NNUEInputs& inputs,
    const NNUEParameters& parameters);

// One fixed training case.
// All scores are in centipawns, positive good for White.
// These are training data, not adjustable network parameters.
struct NNUETrainingExample {
    NNUEInputs inputs;
    float material_score_cp;
    float target_search_score_cp;
};

// Calculate the training aberration for one position:
// aberration = material + network positional score - target search score.
//
// Analogous to calculated optical performance minus desired performance.
// Does not change any parameters.
float calculate_nnue_aberration(
    const NNUETrainingExample& example,
    const NNUEParameters& parameters);

// One Jacobian row: derivatives of ONE example's aberration
// with respect to every network parameter.
// Same layout as NNUEParameters, but contains sensitivities,
// not parameter values.
using NNUEJacobianRow = NNUEParameters;

// Calculate one Jacobian row analytically.
// Material and target scores are fixed, so their derivatives are zero.
// Therefore, aberration derivatives equal network-output derivatives.
// Does not change the network parameters.
void calculate_nnue_jacobian_row(
    const NNUEInputs& inputs,
    const NNUEParameters& parameters,
    NNUEJacobianRow& row);

// So we multiply every entry of your Jacobian row by the aberration.
// Derivatives of the merit function with respect to all parameters.
// Same layout as the parameters, but stores the gradient.
using NNUEGradient = NNUEParameters;

// Calculate one training example's gradient contribution.
// Merit function: L = 0.5 * aberration * aberration.
// Gradient: g[j] = aberration * Jacobian_row[j].
// Does not change the network parameters.
// Returns the aberration for inspection.
float calculate_nnue_gradient(
    const NNUETrainingExample& example,
    const NNUEParameters& parameters,
    NNUEGradient& gradient);

// Apply a gradient-descent parameter adjustment:
// new_parameter = old_parameter - learning_rate * gradient.
// The gradient must have been calculated before any parameters change.
void apply_nnue_gradient(
    NNUEParameters& parameters,
    const NNUEGradient& gradient,
    float learning_rate);

// Add one example's gradient to the accumulated gradient.
// This adds sensitivities of the merit function, not parameter values.
// No network parameter values are changed.
void add_nnue_gradient(
    NNUEGradient& total,
    const NNUEGradient& contribution);

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

    // Whole-position centipawn score, positive good for White.
    // Both overloads leave the stored parameters unchanged.
    float calculate_nnue_positional(const GameBoard& board);
    float calculate_nnue_positional(const NNUEInputs& inputs);

    // Training calculations use the same stored parameters as evaluation.
    // Examples, Jacobian rows, and gradients belong to the training caller.
    float calculate_nnue_aberration(const NNUETrainingExample& example) const;
    void calculate_nnue_jacobian_row(
        const NNUEInputs& inputs,
        NNUEJacobianRow& row) const;
    float calculate_nnue_gradient(
        const NNUETrainingExample& example,
        NNUEGradient& gradient) const;

    // This changes the stored parameters; calculate the gradient first.
    void apply_nnue_gradient(const NNUEGradient& gradient, float learning_rate);

private:
    NNUEParameters parameters_;
};

} // end namespace ShumiChess
