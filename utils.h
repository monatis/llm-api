// Various helper functions and utilities

#pragma once

#include "crow.h"
#include <string>
#include <map>
#include <vector>
#include <random>
#include <thread>

//
// CLI argument parsing
//

struct generation_params
{
    int32_t n_predict = 200;
    int32_t top_k = 40;
    float top_p = 0.9f;
    float temperature = 0.9f;
    int32_t n_batch = 8;
    std::string prompt;
};

struct gpt_params
{
    int32_t seed = -1; // RNG seed
    int32_t n_threads = std::min(4, (int32_t)std::thread::hardware_concurrency());
    generation_params gen_params;             // generation parameters
    std::string model = "ggml-gpt4all-j.bin"; // model path
    int port = 8080;
    bool verbose = false;
};

bool gpt_params_parse(int argc, char **argv, gpt_params &params);

void gpt_print_usage(int argc, char **argv, const gpt_params &params);
generation_params get_generation_params(const generation_params &defaults, const crow::json::rvalue &user_input);

//
// Vocab utils
//

struct gpt_vocab
{
    using id = int32_t;
    using token = std::string;

    std::map<token, id> token_to_id;
    std::map<id, token> id_to_token;
};

void replace(std::string &str, const std::string &needle, const std::string &replacement);

// poor-man's JSON parsing
std::map<std::string, int32_t> json_parse(const std::string &fname);

// split text into tokens
//
// ref: https://github.com/openai/gpt-2/blob/a74da5d99abaaba920de8131d64da2862a8f213b/src/encoder.py#L53
//
// Regex (Python):
// r"""'s|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+"""
//
// Regex (C++):
// R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\s[:alpha:][:digit:]]+|\s+(?!\S)|\s+)"
//
std::vector<gpt_vocab::id> gpt_tokenize(const gpt_vocab &vocab, const std::string &text);

// load the tokens from encoder.json
bool gpt_vocab_init(const std::string &fname, gpt_vocab &vocab);

// sample next token given probabilities for each embedding
//
//   - consider only the top K tokens
//   - from them, consider only the top tokens with cumulative probability > P
//
// TODO: not sure if this implementation is correct
// TODO: temperature is not implemented
//
gpt_vocab::id gpt_sample_top_k_top_p(
    const gpt_vocab &vocab,
    const float *logits,
    int top_k,
    double top_p,
    double temp,
    std::mt19937 &rng);
