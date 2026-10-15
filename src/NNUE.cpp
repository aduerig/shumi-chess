#include "NNUE.hpp"

#include "gameboard.hpp"

#include <cstddef>

namespace ShumiChess {

namespace {

constexpr std::size_t NNUE_INPUT_COUNT = 12 * 64;

struct NNUEActiveInput {
    std::uint16_t feature_index;
    std::uint8_t value;
};

std::size_t collect_active_inputs(
    const NNUEInputs& inputs,
    NNUEActiveInput (&active_inputs)[NNUE_INPUT_COUNT])
{
    std::size_t active_count = 0;

    for (int piece = 0; piece < 12; ++piece) {
        for (int square = 0; square < 64; ++square) {
            const std::uint8_t value = inputs.piece_on_square[piece][square];
            if (value != 0) {
                active_inputs[active_count++] = {
                    static_cast<std::uint16_t>(piece * 64 + square),
                    value
                };
            }
        }
    }

    return active_count;
}

float calculate_unit(
    const NNUEInputs& inputs,
    const NNUEUnitParameters& parameters,
    const NNUEActiveInput* active_inputs,
    std::size_t active_count)
{
    float value = parameters.bias;

    for (std::size_t i = 0; i < active_count; ++i) {
        const NNUEActiveInput& input = active_inputs[i];
        const int piece = input.feature_index >> 6;
        const int square = input.feature_index & 63;
        value += parameters.piece_weight[piece][square] * input.value;
    }

    value += parameters.turn_weight * inputs.black_to_move;

    return value;
}

float activate_unit(float value)
{
    return (value > 0.0f) ? value : 0.0f;
}

} // namespace

NNUE_data::NNUE_data()
    : parameters_{}
{
}

NNUE_data::NNUE_data(const NNUEParameters& initial_parameters)
    : parameters_(initial_parameters)
{
}

const NNUEParameters& NNUE_data::get_parameters() const
{
    return parameters_;
}

void NNUE_data::set_parameters(const NNUEParameters& parameters)
{
    parameters_ = parameters;
}

void NNUE_data::zero_parameters()
{
    parameters_ = {};
}

float NNUE_data::calculate_nnue_positional(const GameBoard& board)
{
    NNUEInputs inputs{};
    fill_nnue_inputs(board, inputs);

    return calculate_nnue_positional(inputs);
}

float NNUE_data::calculate_nnue_positional(const NNUEInputs& inputs)
{
    ++n_calculates;
    return ShumiChess::calculate_nnue_positional(inputs, parameters_);
}

float NNUE_data::calculate_nnue_aberration(const NNUETrainingExample& example) const
{
    return ShumiChess::calculate_nnue_aberration(example, parameters_);
}

void NNUE_data::calculate_nnue_jacobian_row(
    const NNUEInputs& inputs,
    NNUEJacobianRow& row) const
{
    ShumiChess::calculate_nnue_jacobian_row(inputs, parameters_, row);
}

float NNUE_data::calculate_nnue_gradient(
    const NNUETrainingExample& example,
    NNUEGradient& gradient) const
{
    return ShumiChess::calculate_nnue_gradient(example, parameters_, gradient);
}

void NNUE_data::apply_nnue_gradient(const NNUEGradient& gradient, float learning_rate)
{
    ShumiChess::apply_nnue_gradient(parameters_, gradient, learning_rate);
}

void fill_nnue_inputs(const GameBoard& board, NNUEInputs& inputs)
{
    // Set every input to zero.
    inputs = {};

    const ull piece_bitboards[12] = {
        board.white_pawns,
        board.white_knights,
        board.white_bishops,
        board.white_rooks,
        board.white_queens,
        board.white_king,

        board.black_pawns,
        board.black_knights,
        board.black_bishops,
        board.black_rooks,
        board.black_queens,
        board.black_king
    };

    for (int piece_index = 0; piece_index < 12; ++piece_index) {
        const ull bb = piece_bitboards[piece_index];

        for (int square = 0; square < 64; ++square) {
            if (bb & (1ULL << square)) {
                inputs.piece_on_square[piece_index][square] = 1;
            }
        }
    }

    inputs.black_to_move = (board.turn == Color::BLACK) ? 1 : 0;
}

// positional_score = output_bias
//                  + sum over j = 0..31 of:
//                    output_weight[j] * max(0, unit_value[j])

float calculate_nnue_positional(
    const NNUEInputs& inputs,
    const NNUEParameters& parameters)
{
    float score = parameters.output_bias;
    NNUEActiveInput active_inputs[NNUE_INPUT_COUNT];
    const std::size_t active_count = collect_active_inputs(inputs, active_inputs);

    for (int j = 0; j < NNUE_UNIT_COUNT; ++j) {

        const float value = calculate_unit(
            inputs, parameters.units[j], active_inputs, active_count);

        const float activated_value = activate_unit(value);

        score += parameters.output_weight[j] * activated_value;
    }

    return score;
}

// Calculate the training aberration for one position:
// aberration = material + network positional score - target search score.
//
// Analogous to calculated optical performance minus desired performance.
// Does not change any parameters.
float calculate_nnue_aberration(
    const NNUETrainingExample& example,
    const NNUEParameters& parameters)
{



    const float positional_score = calculate_nnue_positional(example.inputs, parameters);

    const float predicted_total = example.material_score_cp + positional_score;

    return predicted_total - example.target_search_score_cp;
}

// Calculate one Jacobian row analytically.
// Material and target scores are fixed, so their derivatives are zero.
// Therefore, aberration derivatives equal network-output derivatives.
// Does not change the network parameters.
void calculate_nnue_jacobian_row(
    const NNUEInputs& inputs,
    const NNUEParameters& parameters,
    NNUEJacobianRow& row)
{
    row = {};
    NNUEActiveInput active_inputs[NNUE_INPUT_COUNT];
    const std::size_t active_count = collect_active_inputs(inputs, active_inputs);

    // Output score contains output_bias directly.
    row.output_bias = 1.0f;

    for (int j = 0; j < NNUE_UNIT_COUNT; ++j) {

        const float value = calculate_unit(
            inputs, parameters.units[j], active_inputs, active_count);

        const float activated_value = activate_unit(value);

        // score contains output_weight[j] * activated_value.
        row.output_weight[j] = activated_value;

        // ReLU derivative is zero for value <= 0.
        if (value <= 0.0f) {
            continue;
        }

        // For positive value, ReLU derivative is 1.
        const float sensitivity = parameters.output_weight[j];

        row.units[j].bias = sensitivity;

        row.units[j].turn_weight =
            sensitivity * inputs.black_to_move;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                row.units[j].piece_weight[piece][square] =
                    sensitivity * inputs.piece_on_square[piece][square];
            }
        }
    }
}

// Calculate one training example's gradient contribution.
// Merit function: L = 0.5 * aberration * aberration.
// Gradient: g[j] = aberration * Jacobian_row[j].
// Does not change the network parameters.
// Returns the aberration for inspection.
float calculate_nnue_gradient(
    const NNUETrainingExample& example,
    const NNUEParameters& parameters,
    NNUEGradient& gradient)
{
    const float aberration =
        calculate_nnue_aberration(example, parameters);

    NNUEJacobianRow row{};
    calculate_nnue_jacobian_row(example.inputs, parameters, row);

    gradient = {};

    gradient.output_bias = aberration * row.output_bias;

    for (int j = 0; j < NNUE_UNIT_COUNT; ++j) {
        gradient.output_weight[j] = aberration * row.output_weight[j];

        gradient.units[j].bias = aberration * row.units[j].bias;

        gradient.units[j].turn_weight = aberration * row.units[j].turn_weight;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                gradient.units[j].piece_weight[piece][square] =
                    aberration * row.units[j].piece_weight[piece][square];
            }
        }
    }

    return aberration;
}

// Apply a gradient-descent parameter adjustment:
// new_parameter = old_parameter - learning_rate * gradient.
// The gradient must have been calculated before any parameters change.
void apply_nnue_gradient(
    NNUEParameters& parameters,
    const NNUEGradient& gradient,
    float learning_rate)
{
    parameters.output_bias -= learning_rate * gradient.output_bias;

    for (int j = 0; j < NNUE_UNIT_COUNT; ++j) {
        parameters.output_weight[j] -= learning_rate * gradient.output_weight[j];

        parameters.units[j].bias -= learning_rate * gradient.units[j].bias;

        parameters.units[j].turn_weight -= learning_rate * gradient.units[j].turn_weight;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                parameters.units[j].piece_weight[piece][square] -=
                    learning_rate
                    * gradient.units[j].piece_weight[piece][square];
            }
        }
    }
}

// Add one example's gradient to the accumulated gradient.
// This adds sensitivities of the merit function, not parameter values.
// No network parameter values are changed.
void add_nnue_gradient(
    NNUEGradient& total,
    const NNUEGradient& contribution)
{
    total.output_bias += contribution.output_bias;

    for (int j = 0; j < NNUE_UNIT_COUNT; ++j) {

        total.output_weight[j] += contribution.output_weight[j];

        total.units[j].bias += contribution.units[j].bias;
        total.units[j].turn_weight += contribution.units[j].turn_weight;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                total.units[j].piece_weight[piece][square] +=
                    contribution.units[j].piece_weight[piece][square];
            }
        }
    }
}

// NNUEParameters myParameters{}; // Everything zero for this check.

// float  calculate_nnue_positional(inputs, myParameters);



void test_NNUE1() {
    const GameBoard board{};
    NNUE_data nnue; // Constructor zeros every parameter for this check.

    const float positional_score = nnue.calculate_nnue_positional(board);
    sout << "test_NNUE" << std::endl;
    assert(positional_score == 0.0f);
}


// // Verify the NNUE calculation using deliberately chosen parameters,
// // with known expected results; no training is performed.
// // Tests one unit's bias and side-to-move weight, followed by ReLU activation,
// // then the output weight and output bias.
// // Expected score: 9 for White to move, 21 for Black to move.
// // Then makes the unit value negative: ReLU must reduce it to zero,
// // leaving only the output bias, for an expected score of 1.
// // Does not test piece-square weights or the board-to-input conversion.
// void test_NNUE1()
// {
//     NNUEInputs inputs{};
//     fill_nnue_inputs(inputs);

//     NNUEParameters parameters{};

//     // Unit 0:
//     // White to move: value = 2.
//     // Black to move: value = 2 + 3 = 5.
//     parameters.units[0].bias = 2.0f;
//     parameters.units[0].turn_weight = 3.0f;

//     // Final score = 1 + 4 * activated unit 0.
//     // Other units contribute zero.
//     parameters.output_weight[0] = 4.0f;
//     parameters.output_bias = 1.0f;

//     float positional_score = calculate_nnue_positional(inputs, parameters);

//     const float expected = inputs.black_to_move ? 21.0f : 9.0f;

//     assert(positional_score == expected);

//     // Make unit 0 negative for either side to move.
//     // Activation must replace its value with zero.
//     parameters.units[0].bias = -10.0f;

//     positional_score = calculate_nnue_positional(inputs, parameters);

//     assert(positional_score == 1.0f);

//     sout << "test_NNUE2 passed" << std::endl;
// }

// // Verify that a piece-square input selects the matching weight.
// // Also verify that an absent piece contributes nothing.
// // No training or board-to-input conversion is tested.
// void test_NNUE1()
// {
//     NNUEInputs inputs{};
//     NNUEParameters parameters{};

//     // Index 1 is White knight in our input ordering.
//     inputs.piece_on_square[1][square_e5] = 1;

//     parameters.units[0].piece_weight[1][square_e5] = 3.0f;

//     // This piece is absent, so its weight must contribute nothing.
//     parameters.units[0].piece_weight[1][square_d5] = 100.0f;

//     parameters.output_weight[0] = 2.0f;

//     // Unit value = 3; activation = 3; output = 2 * 3 = 6.
//     float score = calculate_nnue_positional(inputs, parameters);
//     assert(score == 6.0f);

//     // Remove the knight: all inputs are now zero.
//     inputs.piece_on_square[1][square_e5] = 0;

//     score = calculate_nnue_positional(inputs, parameters);
//     assert(score == 0.0f);

//     sout << "test_NNUE1 passed" << std::endl;
// }

// void test_NNUE1()
// {
//     NNUETrainingExample example{};
//     example.material_score_cp = 100.0f;
//     example.target_search_score_cp = 180.0f;

//     NNUEParameters parameters{};

//     const float aberration = calculate_nnue_aberration(example, parameters);

//     assert(aberration == -80.0f);
// }


// // Verify an analytical Jacobian entry using numerical perturbation.
// // Change one piece-square weight slightly in both directions.
// // Compare the resulting aberration slope with the calculated sensitivity.
// // No training or permanent parameter adjustment is performed.
// void test_NNUE1()
// {
//     NNUETrainingExample example{};
//     example.inputs.piece_on_square[1][square_e5] = 1;
//     example.material_score_cp = 100.0f;
//     example.target_search_score_cp = 110.0f;

//     NNUEParameters parameters{};
//     parameters.units[0].piece_weight[1][square_e5] = 3.0f;
//     parameters.output_weight[0] = 2.0f;

//     NNUEJacobianRow row{};
//     calculate_nnue_jacobian_row(example.inputs, parameters, row);

//     float& parameter = parameters.units[0].piece_weight[1][square_e5];

//     const float original = parameter;

//     // The delta in pataremete space, used to get the jacobean
//     const float epsilon = 0.01f;

//     parameter = original + epsilon;
//     const float aberration_plus =
//         calculate_nnue_aberration(example, parameters);

//     parameter = original - epsilon;
//     const float aberration_minus =
//         calculate_nnue_aberration(example, parameters);

//     parameter = original;

//     const float numerical_derivative =
//         (aberration_plus - aberration_minus)
//         / (2.0f * epsilon);

//     const float analytical_derivative =
//         row.units[0].piece_weight[1][square_e5];

//     assert(std::fabs(numerical_derivative - analytical_derivative) < 0.002f);

//     sout << "Analytical derivative: "
//          << analytical_derivative
//          << ", numerical derivative: "
//          << numerical_derivative << std::endl;
// }

// Verify that one small gradient step reduces the merit function
// for this deliberately constructed training example.
// This is a one-example optimization test, not general training.
// void test_NNUE1()
// {
//     NNUETrainingExample example{};
//     example.inputs.piece_on_square[1][square_e5] = 1;
//     example.material_score_cp = 100.0f;
//     example.target_search_score_cp = 110.0f;

//     NNUEParameters parameters{};
//     parameters.units[0].piece_weight[1][square_e5] = 3.0f;
//     parameters.output_weight[0] = 2.0f;

//     NNUEGradient gradient{};
//     const float before =
//         calculate_nnue_gradient(example, parameters, gradient);

//     apply_nnue_gradient(parameters, gradient, 0.01f);

//     const float after =
//         calculate_nnue_aberration(example, parameters);

//     const float merit_before = 0.5f * before * before;
//     const float merit_after  = 0.5f * after * after;

//     assert(merit_after < merit_before);

//     sout << "Aberration before: " << before
//          << ", after: " << after << std::endl;

//     sout << "Merit before: " << merit_before
//          << ", after: " << merit_after << std::endl;
// }


// // Repeatedly optimize one artificial example.
// // Print progress to observe whether the aberration approaches zero.
// // This demonstrates optimization, not learning across different positions.
// void test_NNUE1()
// {
//     NNUETrainingExample example{};
//     example.inputs.piece_on_square[1][square_e5] = 1;
//     example.material_score_cp = 100.0f;
//     example.target_search_score_cp = 110.0f;

//     NNUEParameters parameters{};
//     parameters.units[0].piece_weight[1][square_e5] = 3.0f;
//     parameters.output_weight[0] = 2.0f;

//     const float learning_rate = 0.01f;

//     for (int iteration = 0; iteration <= 100; ++iteration) {
//         NNUEGradient gradient{};

//         const float aberration = calculate_nnue_gradient(example, parameters, gradient);

//         const float merit = 0.5f * aberration * aberration;

//         if (iteration % 10 == 0) {
//             sout << "Iteration: " << iteration
//                  << " aberration: " << aberration
//                  << " merit: " << merit << std::endl;
//         }

//         if (iteration < 100) {
//             apply_nnue_gradient(parameters, gradient, learning_rate);
//         }
//     }
// }



///////////////////////////////////////////////////////////////////////////////////////

} // end namespace ShumiChess
