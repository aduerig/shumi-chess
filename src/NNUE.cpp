#include "NNUE.hpp"

#include "gameboard.hpp"
#include "utility.hpp"

#include <cstddef>


#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
    #undef NDEBUG
#endif   
#include <assert.h>

//////////////////////////////////////////////////////////////////////////////////////////////

namespace ShumiChess {

std::size_t NNUE_data::collect_active_inputs(
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


std::size_t NNUE_data::collect_active_inputs_from_bitboards(
    const ull (&piece_bitboards)[12],       // input
    NNUEActiveInput* active_inputs)         // output
{
    std::size_t count = 0;

    for (int piece = 0; piece < 12; ++piece) {
        ull bb = piece_bitboards[piece];

        while (bb) {
            const Square square = utility::bit::lsb_and_pop_to_square(bb);

            int feature_index = (piece * 64) + square;

            active_inputs[count].feature_index = (std::uint16_t)feature_index;
            active_inputs[count].value = 1;

            count++;
        }
    }

    return count;
}




float NNUE_data::calculate_neuron(
    const NNUEInputs& inputs,
    const NNUENeuronParameters& parameters,
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


/////////////////////////////////////////////////////////////////////////////////////

NNUE_data::NNUE_data()
    : parameters_{},
      inference_parameters_(std::make_shared<NNUEInferenceParameters>())
{
}

NNUE_data::NNUE_data(const NNUEParameters& initial_parameters)
    : parameters_(initial_parameters)
{
    rebuild_inference_parameters_();
}

const NNUEParameters& NNUE_data::get_parameters() const
{
    return parameters_;
}

void NNUE_data::set_parameters(const NNUEParameters& parameters)
{
    parameters_ = parameters;
    rebuild_inference_parameters_();
}

void NNUE_data::zero_parameters()
{
    parameters_ = {};
    rebuild_inference_parameters_();
}

void NNUE_data::rebuild_inference_parameters_()
{
    auto inference = std::make_shared<NNUEInferenceParameters>();

    for (int piece = 0; piece < 12; ++piece) {
        for (int square = 0; square < 64; ++square) {
            for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
                inference->piece_weight[piece][square][j] =
                    parameters_.Neurons[j].piece_weight[piece][square];
            }
        }
    }

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
        inference->bias[j] = parameters_.Neurons[j].bias;
        inference->turn_weight[j] = parameters_.Neurons[j].turn_weight;
        inference->output_weight[j] = parameters_.output_weight[j];
    }
    inference->output_bias = parameters_.output_bias;

    inference_parameters_ = inference;
}

////////////////////////////////////////////////////////////////////////////////////

float NNUE_data::Calculate_nnue_positional(const GameBoard& board)
{
    const ull piece_bitboards[12] = {
        board.white_pawns, board.white_knights, board.white_bishops,
        board.white_rooks, board.white_queens, board.white_king,
        board.black_pawns, board.black_knights, board.black_bishops,
        board.black_rooks, board.black_queens, board.black_king
    };

    NNUEActiveInput active_inputs[NNUE_INPUT_COUNT];

    const std::size_t active_count = collect_active_inputs_from_bitboards(piece_bitboards, 
                active_inputs);     // output

    const std::uint8_t black_to_move = (board.turn == Color::BLACK) ? 1 : 0;

    ++n_calculates;

    return calculate_nnue_positional_(black_to_move, *inference_parameters_, active_inputs, active_count);
}

float NNUE_data::Calculate_nnue_aberration(const NNUETrainingExample& example)
{
    return calculate_nnue_aberration_(example, parameters_);
}

void NNUE_data::calculate_nnue_jacobian_row(
    const NNUEInputs& inputs,
    NNUEJacobianRow& row) const
{
    calculate_nnue_jacobian_row_(inputs, parameters_, row);
}

// called ony in testing
float NNUE_data::calculate_nnue_gradient(
    const NNUETrainingExample& example,
    NNUEGradient& gradient) const
{
    return _calculate_nnue_gradient(example, parameters_, gradient);
}

// called only in testing
void NNUE_data::apply_nnue_gradient(const NNUEGradient& gradient, float learning_rate)
{
    apply_nnue_gradient_(parameters_, gradient, learning_rate);
    rebuild_inference_parameters_();
}

void NNUE_data::fill_nnue_inputs(const GameBoard& board, NNUEInputs& inputs) const
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

void NNUE_data::fill_nnue_inputs_fast(const GameBoard& board, NNUEInputs& inputs) const
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
        ull bb = piece_bitboards[piece_index];

        while (bb) {
            const Square square = utility::bit::lsb_and_pop_to_square(bb);
            inputs.piece_on_square[piece_index][square] = 1;
        }
    }

    inputs.black_to_move = (board.turn == Color::BLACK) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////////

// positional_score = output_bias
//                  + sum over j = 0..31 of:
//                    output_weight[j] * max(0, unit_value[j])

float NNUE_data::calculate_nnue_positional_(
    std::uint8_t black_to_move,
    const NNUEInferenceParameters& parameters,
    const NNUEActiveInput* active_inputs,
    std::size_t active_count)
{
    alignas(64) float values[NNUE_NEURON_COUNT];

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
        values[j] = parameters.bias[j];
    }

    // Each inner loop reads consecutive weights and updates independent neurons.
    // Preserve the original feature order within each neuron's sum.
    for (std::size_t i = 0; i < active_count; ++i) {

        const NNUEActiveInput& input = active_inputs[i];
        const int piece = input.feature_index >> 6;
        const int square = input.feature_index & 63;
        const float* weights = parameters.piece_weight[piece][square];

        const float input_value = (float)(input.value);

        assert (input_value == 1.0f);
        for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
            values[j] += (weights[j] * input_value);
        }
    }

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
        values[j] += parameters.turn_weight[j] * black_to_move;
    }

    float score = parameters.output_bias;
    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
        score += parameters.output_weight[j] * activate_unit(values[j]);
    }

    return score;
}

// Training uses the original parameter layout and scalar neuron calculation.
float NNUE_data::calculate_nnue_positional_training(
    const NNUEInputs& inputs,
    const NNUEParameters& parameters)
{
    float score = parameters.output_bias;

    NNUEActiveInput active_inputs[NNUE_INPUT_COUNT];
    const std::size_t active_count = collect_active_inputs(inputs, active_inputs);

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {

        const float value = calculate_neuron(
            inputs, parameters.Neurons[j], active_inputs, active_count);

        // You can think of the neuron as becoming active when its sum is positive.
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
float NNUE_data::calculate_nnue_aberration_(
    const NNUETrainingExample& example,
    const NNUEParameters& parameters)
{



    const float positional_score = NNUE_data::calculate_nnue_positional_training(example.inputs, parameters);

    const float predicted_total = example.material_score_cp + positional_score;

    return predicted_total - example.target_search_score_cp;
}

// Calculate one Jacobian row analytically.
// Material and target scores are fixed, so their derivatives are zero.
// Therefore, aberration derivatives equal network-output derivatives.
// Does not change the network parameters.
void NNUE_data::calculate_nnue_jacobian_row_(
    const NNUEInputs& inputs,
    const NNUEParameters& parameters,
    NNUEJacobianRow& row)
{
    row = {};

    NNUEActiveInput active_inputs[NNUE_INPUT_COUNT];
    const std::size_t active_count = collect_active_inputs(inputs, active_inputs);

    // Output score contains output_bias directly.
    row.output_bias = 1.0f;

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {

        const float value = calculate_neuron(
            inputs, parameters.Neurons[j], active_inputs, active_count);

        const float activated_value = activate_unit(value);

        // score contains output_weight[j] * activated_value.
        row.output_weight[j] = activated_value;

        // ReLU derivative is zero for value <= 0.
        if (value <= 0.0f) {
            continue;
        }

        // For positive value, ReLU derivative is 1.
        const float sensitivity = parameters.output_weight[j];

        row.Neurons[j].bias = sensitivity;

        row.Neurons[j].turn_weight = sensitivity * inputs.black_to_move;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                row.Neurons[j].piece_weight[piece][square] =
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
float NNUE_data::_calculate_nnue_gradient(
    const NNUETrainingExample& example,
    const NNUEParameters& parameters,
    NNUEGradient& gradient)
{
    const float positional_score = calculate_nnue_positional_training(example.inputs, parameters);
    const float predicted_total = example.material_score_cp + positional_score;
    const float aberration = predicted_total - example.target_search_score_cp;

    NNUEJacobianRow row{};
    calculate_nnue_jacobian_row_(example.inputs, parameters, row);

    gradient = {};

    gradient.output_bias = aberration * row.output_bias;

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
        gradient.output_weight[j] = aberration * row.output_weight[j];

        gradient.Neurons[j].bias = aberration * row.Neurons[j].bias;

        gradient.Neurons[j].turn_weight = aberration * row.Neurons[j].turn_weight;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                gradient.Neurons[j].piece_weight[piece][square] =
                    aberration * row.Neurons[j].piece_weight[piece][square];
            }
        }
    }

    return aberration;
}

// Apply a gradient-descent parameter adjustment:
// new_parameter = old_parameter - learning_rate * gradient.
// The gradient must have been calculated before any parameters change.
void NNUE_data::apply_nnue_gradient_(
    NNUEParameters& parameters,
    const NNUEGradient& gradient,
    float learning_rate)
{
    parameters.output_bias -= learning_rate * gradient.output_bias;

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {
        parameters.output_weight[j] -= learning_rate * gradient.output_weight[j];

        parameters.Neurons[j].bias -= learning_rate * gradient.Neurons[j].bias;

        parameters.Neurons[j].turn_weight -= learning_rate * gradient.Neurons[j].turn_weight;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                parameters.Neurons[j].piece_weight[piece][square] -=
                    learning_rate
                    * gradient.Neurons[j].piece_weight[piece][square];
            }
        }
    }
}

// // Add one example's gradient to the accumulated gradient.
// // This adds sensitivities of the merit function, not parameter values.
// // No network parameter values are changed.
// void add_nnue_gradient(
//     NNUEGradient& total,
//     const NNUEGradient& contribution)
// {
//     total.output_bias += contribution.output_bias;

//     for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {

//         total.output_weight[j] += contribution.output_weight[j];

//         total.Neurons[j].bias += contribution.Neurons[j].bias;
//         total.Neurons[j].turn_weight += contribution.Neurons[j].turn_weight;

//         for (int piece = 0; piece < 12; ++piece) {
//             for (int square = 0; square < 64; ++square) {
//                 total.Neurons[j].piece_weight[piece][square] +=
//                     contribution.Neurons[j].piece_weight[piece][square];
//             }
//         }
//     }
// }

// NNUEParameters myParameters{}; // Everything zero for this check.


void test_NNUE1() {
    const GameBoard board{};
    NNUE_data nnue; // Constructor zeros every parameter for this check.

    const float positional_score = nnue.Calculate_nnue_positional(board);
    sout << "test_NNUE" << std::endl;
    assert(positional_score == 0.0f);
}

void test_NNUE_training_one_position()
{
    const GameBoard board{};

    NNUETrainingExample example{};
    NNUE_data nnue;

    // "inputs" <- the board
    nnue.fill_nnue_inputs_fast(board, example.inputs);

    // Artificial training target: NNUE should learn +75 cp.
    example.material_score_cp = 0.0f;
    example.target_search_score_cp = 75.0f;

    // Initialize one neuron so its ReLU is active and can learn.
    NNUEParameters parameters{};

    parameters.Neurons[0].bias = 1.0f;
    parameters.output_weight[0] = 1.0f;

    nnue.set_parameters(parameters);

    // Calculate one row of the training chart.
    const int iteration = 0;

    const float aberration = nnue.Calculate_nnue_aberration(example);

    const float nnue_positional =
        example.target_search_score_cp
        - example.material_score_cp
        + aberration;

    sout << "Iteration: " << iteration
         << "  NNUE positional: " << nnue_positional
         << "  Aberration: " << aberration
         << std::endl;
}

// // Verify the NNUE calculation using deliberately chosen parameters,
// // with known expected results; no training is performed.
// // Tests one neurons bias and side-to-move weight, followed by ReLU activation,
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
//     parameters.Neurons[0].bias = 2.0f;
//     parameters.Neurons[0].turn_weight = 3.0f;

//     // Final score = 1 + 4 * activated unit 0.
//     // Other units contribute zero.
//     parameters.output_weight[0] = 4.0f;
//     parameters.output_bias = 1.0f;

//     float positional_score = calculate_nnue_positional(inputs, parameters);

//     const float expected = inputs.black_to_move ? 21.0f : 9.0f;

//     assert(positional_score == expected);

//     // Make neuron 0 negative for either side to move.
//     // Activation must replace its value with zero.
//     parameters.Neurons[0].bias = -10.0f;

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

//     parameters.Neurons[0].piece_weight[1][square_e5] = 3.0f;

//     // This piece is absent, so its weight must contribute nothing.
//     parameters.Neurons[0].piece_weight[1][square_d5] = 100.0f;

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

//     const float aberration = Calculate_nnue_aberration(example, parameters);

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
//     parameters.Neurons[0].piece_weight[1][square_e5] = 3.0f;
//     parameters.output_weight[0] = 2.0f;

//     NNUEJacobianRow row{};
//     calculate_nnue_jacobian_row(example.inputs, parameters, row);

//     float& parameter = parameters.Neurons[0].piece_weight[1][square_e5];

//     const float original = parameter;

//     // The delta in pataremete space, used to get the jacobean
//     const float epsilon = 0.01f;

//     parameter = original + epsilon;
//     const float aberration_plus =
//         Calculate_nnue_aberration(example, parameters);

//     parameter = original - epsilon;
//     const float aberration_minus =
//         Calculate_nnue_aberration(example, parameters);

//     parameter = original;

//     const float numerical_derivative =
//         (aberration_plus - aberration_minus)
//         / (2.0f * epsilon);

//     const float analytical_derivative =
//         row.Neurons[0].piece_weight[1][square_e5];

//     assert(std::fabs(numerical_derivative - analytical_derivative) < 0.002f);

//     sout << "Analytical derivative: "
//          << analytical_derivative
//          << ", numerical derivative: "
//          << numerical_derivative << std::endl;
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
//     parameters.Neurons[0].piece_weight[1][square_e5] = 3.0f;
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
