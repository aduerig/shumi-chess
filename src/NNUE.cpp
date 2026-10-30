
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include "NNUE.hpp"

#include "engine.hpp"
#include "fen_database.hpp"
#include "gameboard.hpp"
#include "utility.hpp"
#include "minimax.hpp"




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

// Watch out i use a sqrt()
double NNUE_data::Calculate_nnue_RMSe(const std::vector<NNUETrainingExample>& examples)
{
    return Calculate_nnue_RMSe(examples.data(), examples.size());
}

double NNUE_data::Calculate_nnue_RMSe(
    const NNUETrainingExample* examples,
    std::size_t count)
{
    if (count == 0) {
        return 0.0;
    }

    double sum_squared_aberrations = 0.0;
    for (std::size_t position = 0; position < count; ++position) {
        const double aberration = Calculate_nnue_aberration(examples[position]);
        sum_squared_aberrations += aberration * aberration;
    }

    return std::sqrt(sum_squared_aberrations / static_cast<double>(count));
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



float NNUE_data::calculate_nnue_aberration_(
    const NNUETrainingExample& example,
    const NNUEParameters& parameters)
{
    const float positional_score = NNUE_data::calculate_nnue_positional_training(example.inputs, parameters);

    return (positional_score - example.target_positional_score_cp);
}




// Calculate one Jacobian row analytically. (the orginal Jacobian is all one col, but the Jt (J transpose) is all one row)
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

    const float aberration = positional_score - example.target_positional_score_cp;


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

//
// Computes J * Jtranspose, for J a Jacobean matrix.
float NNUE_data::calculate_JJt(const NNUEInputs& inputs) const
{
    NNUEJacobianRow row{};
    calculate_nnue_jacobian_row(inputs, row);

    float JJt = row.output_bias * row.output_bias;

    for (int j = 0; j < NNUE_NEURON_COUNT; ++j) {

        JJt += row.output_weight[j] * row.output_weight[j];
        JJt += row.Neurons[j].bias * row.Neurons[j].bias;
        JJt += row.Neurons[j].turn_weight *
               row.Neurons[j].turn_weight;

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                float derivative = row.Neurons[j].piece_weight[piece][square];
                JJt += derivative * derivative;
            }
        }
    }

    return JJt;
}


//
// Count training positions for which all hidden neurons are ReLU-locked.
// JJt == 1 means only the output bias has a nonzero derivative.
// Does not modify the NNUE parameters.
int NNUE_data::count_locked_positions(
    const NNUETrainingExample* examples,
    int NPositions) const
{
    int locked_count = 0;

    for (int position = 0; position < NPositions; ++position) {
        const float JJt = calculate_JJt(examples[position].inputs);

        if (JJt == 1.0f) ++locked_count;
    }

    return locked_count;
}




void NNUE_data::initialize_parameters()
{
    // Local storage for one full set of starting weights and biases; {} zeros them all.
    // The nnue object already owns separate working storage.
    NNUEParameters parameters{};

    // Neuron 0: z[0] = bias + sum(piece_weight[p][s] * piece_on_square[p][s])
    //                     + turn_weight * black_to_move. Its activation is max(0, z[0]).
    // All its input weights start at zero, so bias = 1 gives activation = max(0, 1) = 1.
    parameters.Neurons[0].bias = 20.0f;

    // Final score = output_bias + sum over neurons j of (output_weight[j] * activation[j]).
    // This weight scales neuron 0's activation; initially the score is 0 + 1 * 1 = 1 cp.
    // All other neurons contribute zero because their parameters are still zero.
    parameters.output_weight[0] = 1.0f;

    parameters.Neurons[1].bias = 5.0f;
    parameters.output_weight[1] = 0.9f;

//    parameters.Neurons[2].bias = 1.0f;
//     parameters.output_weight[2] = 1.9f;

    // Copy these starting values into nnue's own parameters and rebuild its fast-evaluation copy.
    // Later training updates nnue's copy; this local parameters object stays unchanged.
    set_parameters(parameters);
}


void NNUE_data::initialize_parameters_bounded(int neuron_count, const NNUEInitializationBounds& bounds)
{
    assert(neuron_count >= 0 && neuron_count <= NNUE_NEURON_COUNT);

    // Local storage for one full set of starting weights and biases; {} zeros them all.
    // The nnue object already owns separate working storage.
    NNUEParameters parameters{};

    for (int j = 0; j < neuron_count; ++j) {

        parameters.Neurons[j].bias = utility::random_float(-bounds.neuron_bias, bounds.neuron_bias);
        parameters.Neurons[j].turn_weight = utility::random_float(-bounds.turn_weight, bounds.turn_weight);
        parameters.output_weight[j] = utility::random_float(-bounds.output_weight, bounds.output_weight);

        for (int piece = 0; piece < 12; ++piece) {
            for (int square = 0; square < 64; ++square) {
                parameters.Neurons[j].piece_weight[piece][square] =
                    utility::random_float(-bounds.piece_weight, bounds.piece_weight);
            }
        }
    }

    parameters.output_bias = 0.0f;

    // Copy these starting values into nnue's own parameters and rebuild its fast-evaluation copy.
    // Later training updates nnue's copy; this local parameters object stays unchanged.
    set_parameters(parameters);
}



void test_NNUE1() {
    const GameBoard board{};
    NNUE_data nnue; // Constructor zeros every parameter for this check.

    const float positional_score = nnue.Calculate_nnue_positional(board);
    sout << "test_NNUE" << std::endl;
    assert(positional_score == 0.0f);
}


//
//  This is the same basic iterative least-squares framework you know from optical design, except 
//  that you're using gradient descent rather than solving normal equations.
void test_NNUE_training_one_position(MinimaxAI* pPlayer)
{
    constexpr float EPS = 0.01f;
    constexpr int TEST_RECORD_COUNT = 5;

    // Controls each weight/bias adjustment: new_value = old_value - learning_rate * its_gradient.
    // Smaller rates give smaller steps; a rate that is too large can overshoot and increase the error.
    constexpr float learning_rate = 0.001f;

    // Setup the starting chess position
    const GameBoard& board = pPlayer->engine.game_board;
    //const GameBoard board{};

    // Store the fixed board inputs, and positional score for this example.
    NNUETrainingExample example{};

    // Owns the working weights/biases and a separate heap copy arranged for fast evaluation.
    NNUE_data nnue;

    // Convert the board into the piece-square and side-to-move inputs used during training.
    nnue.fill_nnue_inputs_fast(board, example.inputs);

    ShumiChess::EvalPersons eval_person = ShumiChess::UNCLE_SHUMI;

    int material_score_cp;          /* Shumi's material evaluation */
    int positional_score_cp;        /* Shumi's positional evaluation */

    // Notice that total_score_cp is not used. The NNUE only does the positional part of the evaluator.

    // Show board
    // string out = utility::representation::gameboard_to_string(board);
    // sout << out << endl;

    int total_score_cp = pPlayer->evaluate_board_t<ShumiChess::Color::WHITE>(eval_person
                , &material_score_cp, &positional_score_cp);    // outputs

    example.target_positional_score_cp = (float)positional_score_cp;
   
    //example.target_positional_score_cp = 6.0f;      // debug only 

    // sout << "trg=" << total_score_cp << " wee=" << material_score_cp << " zee=" << positional_score_cp << endl;

    nnue.initialize_parameters();

    // Storage for the derivatives of the loss with respect to each NNUE parameter.
    // These derivatives are recalculated during every training iteration.
    NNUEGradient gradient{};

    int convergence_count = 0;
    int jjt_lock_count = 0;

    constexpr int TRAINING_ITERATIONS = 50;
    for (int iteration = 0; iteration < TRAINING_ITERATIONS; ++iteration) {

        for (int position = 0; position < 1; ++position) {

            // Calculate the aberration: NNUE positional score minus
            // Shumi's target positional score. No parameters change.
            const float aberration = nnue.Calculate_nnue_aberration(example);

            // Calculate NNUE's current positional score for display.
            const float nnue_positional =
                example.target_positional_score_cp + aberration;

            // Print the NNUE positional score and aberration using the current
            // parameters, before applying this iteration's gradient correction.
            float fff = nnue.calculate_JJt(example.inputs);
            // std::ostringstream aberration_text;
            // aberration_text << std::fixed << std::setprecision(10) << aberration;
            // sout << "Iter: " << iteration
            //         << " NNUE posit: " << nnue_positional
            //         << " Aberration: " << aberration_text.str()
            //         << " JJt: " << fff
            //         << std::endl;

            if (std::fabs(aberration) < EPS) {
                ++convergence_count;
            } else {
                convergence_count = 0;
            }

            if (std::fabs(fff - 1.0f) < EPS) {
                ++jjt_lock_count;
            } else {
                jjt_lock_count = 0;
            }

            //  
            // The Jacobian J contains derivatives da/dp, where:
            //   a = aberration (predicted score minus target score)
            //   p = adjustable NNUE parameter (weight or bias)
            //
            // Compute the gradient of the least-squares loss at the current
            // NNUE parameter values.
            //
            // For loss L = (1/2) * sum(a*a):
            //   dL/dp = sum(a * da/dp)
            //
            // In matrix notation:
            //   gradient(L) = J_transpose * e
            //   The code calculates this product without constructing the full "J" matrix.
            //   (where e is the vector of aberrations)
            //
            nnue.calculate_nnue_gradient(example, gradient);

            // Subtract learning_rate * gradient from nnue's parameters and refresh its evaluation copy.
            nnue.apply_nnue_gradient(gradient, learning_rate);

        }   // END loop over positions

    }   // END loop over training iterations

    // sout << "NNUE Training Summary: Convergence="
    //      << (convergence_count >= TEST_RECORD_COUNT ? "PASS" : "FAIL")
    //      << "  Activation="
    //      << (jjt_lock_count >= TEST_RECORD_COUNT ? "FAIL" : "PASS")
    //      << std::endl;

    Engine& engine = pPlayer->engine;
    ++engine.NNUE_attempts;
    if (convergence_count >= TEST_RECORD_COUNT) {
        ++engine.NNUE_convergence_passes;
    }
    if (jjt_lock_count < TEST_RECORD_COUNT) {
        ++engine.NNUE_activation_passes;
    }

}   // END test



int multi_FEN_readin(
    const std::filesystem::path& filename,
    std::vector<NNUETrainingExample>& examples,
    NNUETrainingMethod iMethod,
    std::string* error)
{
    if (error) {
        error->clear();
    }
    const std::string filename_text = filename.string();
    const auto fail = [&](const std::string& reason) {
        const std::string diagnostic = filename_text + ": " + reason;
        if (error) {
            *error = diagnostic;
        } else {
            std::cerr << diagnostic << '\n';
        }
        return EXIT_FAILURE;
    };

    if (iMethod != NNUETrainingMethod::STATIC_EVAL &&
        iMethod != NNUETrainingMethod::FULL_SEARCH) {
        return fail("unknown NNUE training method");
    }

    std::vector<FEN_Score> positions;
    if (!read_fen_database(filename_text, positions, error)) {
        return EXIT_FAILURE;
    }
    if (positions.empty()) {
        return fail("no usable FEN positions");
    }
    // The training routine accepts an int position count.
    if (positions.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return fail("position count exceeds the training routine's int range");
    }

    std::vector<NNUETrainingExample> loaded(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i) {
        try {
            const FEN_Score& position = positions[i];
            Engine engine(position.fen);
            MinimaxAI minimax_ai(engine);
            minimax_ai.nnue.fill_nnue_inputs_fast(engine.game_board, loaded[i].inputs);

            if (iMethod == NNUETrainingMethod::STATIC_EVAL ||
                position.score == HUGE_SCORE || IS_MATE_SCORE(position.score)) {
                int material_score_cp;
                int positional_score_cp;
                minimax_ai.evaluate_board_t<Color::WHITE>(
                    UNCLE_SHUMI, &material_score_cp, &positional_score_cp);
                loaded[i].target_positional_score_cp = static_cast<float>(positional_score_cp);
            } else {
                loaded[i].target_positional_score_cp =
                    static_cast<float>(position.score) - minimax_ai.get_material();
            }
        } catch (const std::exception& exception) {
            return fail("record " + std::to_string(i + 1) + ": " + exception.what());
        }
    }

    examples.swap(loaded);
    return EXIT_SUCCESS;
}

void test_NNUE_training_mult_positions(
    NNUE_data& nnue,                        // input
    const NNUETrainingExample* examples,    // input
    int NPositions                          // input
) {
    constexpr float learning_rate = 0.001f;

    // Initialize the NNUE parameters once, before training.
    nnue.initialize_parameters();

    const std::size_t position_count = NPositions > 0 ? static_cast<std::size_t>(NPositions) : 0;
    const double initial_rmse = nnue.Calculate_nnue_RMSe(examples, position_count);
    sout << "Initial RMSe: " << initial_rmse << " cp" << std::endl;

    // Storage for the derivatives of the loss.
    NNUEGradient gradient{};

    constexpr int TRAINING_ITERATIONS = 100;

    for (int iteration = 0; iteration < TRAINING_ITERATIONS; ++iteration) {

        for (int position = 0; position < NPositions; ++position) {

            const NNUETrainingExample& example = examples[position];

            // Calculate the aberration for this position.
            const float aberration = nnue.Calculate_nnue_aberration(example);
            
            std::ostringstream aberration_text;
            aberration_text << std::showpos << std::fixed << std::setprecision(5) << aberration;

            float JJt = nnue.calculate_JJt(example.inputs);

            if ( (iteration == 0) || (iteration == TRAINING_ITERATIONS-1) ) {
                sout << std::setw(4) << iteration
                    << "  Pos: " << position
                    << "  Abrr:" << aberration_text.str()
                    << "  JJt  " << JJt
                    //<< "  Targ " << example.target_positional_score_cp
                    //<< "  Pred " << (example.target_positional_score_cp + aberration)
                    << std::endl;
            }

            // Calculate the gradient for this position.
            nnue.calculate_nnue_gradient(example, gradient);

            // Update the same NNUE parameters.
            nnue.apply_nnue_gradient(gradient, learning_rate);

        }   // END loop over positions

    }   // END loop over training iterations

    const double final_rmse = nnue.Calculate_nnue_RMSe(examples, position_count);
    int nlocks = nnue.count_locked_positions(examples, (int)position_count);
    sout << "Final RMSe=" << final_rmse << " nlocks=" << nlocks << std::endl;
}

 

///////////////////////////////////////////////////////////////////////////////////////

} // end namespace ShumiChess
