#include "utils.h"

#include <fstream>
#include <regex>

bool gpt_params_parse(int argc, char **argv, gpt_params &params)
{
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-s" || arg == "--seed")
        {
            params.seed = std::stoi(argv[++i]);
        }
        else if (arg == "-t" || arg == "--threads")
        {
            params.n_threads = std::stoi(argv[++i]);
        }
        else if (arg == "-p" || arg == "--prompt")
        {
            params.gen_params.prompt = argv[++i];
        }
        else if (arg == "--port")
        {
            params.port = std::stoi(argv[++i]);
        }
        else if (arg == "-n" || arg == "--n_predict")
        {
            params.gen_params.n_predict = std::stoi(argv[++i]);
        }
        else if (arg == "--top_k")
        {
            params.gen_params.top_k = std::stoi(argv[++i]);
        }
        else if (arg == "--top_p")
        {
            params.gen_params.top_p = std::stof(argv[++i]);
        }
        else if (arg == "--temperature")
        {
            params.gen_params.temperature = std::stof(argv[++i]);
        }
        else if (arg == "-b" || arg == "--batch_size")
        {
            params.gen_params.n_batch = std::stoi(argv[++i]);
        }
        else if (arg == "-m" || arg == "--model")
        {
            params.model = argv[++i];
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            params.verbose = true;
        }
        else if (arg == "-h" || arg == "--help")
        {
            gpt_print_usage(argc, argv, params);
            exit(0);
        }
        else
        {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            gpt_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void gpt_print_usage(int argc, char **argv, const gpt_params &params)
{
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h, --help            show this help message and exit\n");
    fprintf(stderr, "  -v, --verbose            log generation in stdout (default: disabled)\n");
    fprintf(stderr, "  -s SEED, --seed SEED  RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N, --threads N     number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "  --port PORT     port to listen on (default: %d)\n", params.port);
    fprintf(stderr, "  -p PROMPT, --prompt PROMPT\n");
    fprintf(stderr, "                        prompt to start generation with (default: random)\n");
    fprintf(stderr, "  -n N, --n_predict N   number of tokens to predict (default: %d)\n", params.gen_params.n_predict);
    fprintf(stderr, "  --top_k N             top-k sampling (default: %d)\n", params.gen_params.top_k);
    fprintf(stderr, "  --top_p N             top-p sampling (default: %.1f)\n", params.gen_params.top_p);
    fprintf(stderr, "  --temp N              temperature (default: %.1f)\n", params.gen_params.temperature);
    fprintf(stderr, "  -b N, --batch_size N  batch size for prompt processing (default: %d)\n", params.gen_params.n_batch);
    fprintf(stderr, "  -m FNAME, --model FNAME\n");
    fprintf(stderr, "                        model path (default: %s)\n", params.model.c_str());
    fprintf(stderr, "\n");
}

generation_params get_generation_params(const generation_params &defaults, const crow::json::rvalue &user_input)
{
    generation_params params = defaults;

    if (user_input.has("n_predict"))
    {
        params.n_predict = user_input["n_predict"].i();
    }

    if (user_input.has("top_k"))
    {
        params.top_k = user_input["top_k"].i();
    }

    if (user_input.has("top_p"))
    {
        params.top_p = user_input["top_p"].d();
    }

    if (user_input.has("temperature"))
    {
        params.temperature = user_input["temperature"].d();
    }

    if (user_input.has("n_batch"))
    {
        params.n_batch = user_input["n_batch"].i();
    }

    if (user_input.has("prompt"))
    {
        params.prompt = user_input["prompt"].s();
    }

    return params;
}

void replace(std::string &str, const std::string &needle, const std::string &replacement)
{
    size_t pos = 0;
    while ((pos = str.find(needle, pos)) != std::string::npos)
    {
        str.replace(pos, needle.length(), replacement);
        pos += replacement.length();
    }
}

std::map<std::string, int32_t> json_parse(const std::string &fname)
{
    std::map<std::string, int32_t> result;

    // read file into string
    std::string json;
    {
        std::ifstream ifs(fname);
        if (!ifs)
        {
            fprintf(stderr, "Failed to open %s\n", fname.c_str());
            exit(1);
        }

        json = std::string((std::istreambuf_iterator<char>(ifs)),
                           (std::istreambuf_iterator<char>()));
    }

    if (json[0] != '{')
    {
        return result;
    }

    // parse json
    {
        bool has_key = false;
        bool in_token = false;

        std::string str_key = "";
        std::string str_val = "";

        int n = json.size();
        for (int i = 1; i < n; ++i)
        {
            if (!in_token)
            {
                if (json[i] == ' ')
                    continue;
                if (json[i] == '"')
                {
                    in_token = true;
                    continue;
                }
            }
            else
            {
                if (json[i] == '\\' && i + 1 < n)
                {
                    if (has_key == false)
                    {
                        str_key += json[i];
                    }
                    else
                    {
                        str_val += json[i];
                    }
                    ++i;
                }
                else if (json[i] == '"')
                {
                    if (has_key == false)
                    {
                        has_key = true;
                        ++i;
                        while (json[i] == ' ')
                            ++i;
                        ++i; // :
                        while (json[i] == ' ')
                            ++i;
                        if (json[i] != '\"')
                        {
                            while (json[i] != ',' && json[i] != '}')
                            {
                                str_val += json[i++];
                            }
                            has_key = false;
                        }
                        else
                        {
                            in_token = true;
                            continue;
                        }
                    }
                    else
                    {
                        has_key = false;
                    }

                    ::replace(str_key, "\\u0120", " ");  // \u0120 -> space
                    ::replace(str_key, "\\u010a", "\n"); // \u010a -> new line
                    ::replace(str_key, "\\\"", "\"");    // \\\"   -> "

                    try
                    {
                        result[str_key] = std::stoi(str_val);
                    }
                    catch (...)
                    {
                        // fprintf(stderr, "%s: ignoring key '%s' with value '%s'\n", fname.c_str(), str_key.c_str(), str_val.c_str());
                    }
                    str_key = "";
                    str_val = "";
                    in_token = false;
                    continue;
                }
                if (has_key == false)
                {
                    str_key += json[i];
                }
                else
                {
                    str_val += json[i];
                }
            }
        }
    }

    return result;
}

std::vector<gpt_vocab::id> gpt_tokenize(const gpt_vocab &vocab, const std::string &text)
{
    std::vector<std::string> words;

    // first split the text into words
    {
        std::string str = text;
        std::string pat = R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\s[:alpha:][:digit:]]+|\s+(?!\S)|\s+)";

        std::regex re(pat);
        std::smatch m;

        while (std::regex_search(str, m, re))
        {
            for (auto x : m)
            {
                words.push_back(x);
            }
            str = m.suffix();
        }
    }

    // find the longest tokens that form the words:
    std::vector<gpt_vocab::id> tokens;
    for (const auto &word : words)
    {
        if (word.size() == 0)
            continue;

        int i = 0;
        int n = word.size();
        while (i < n)
        {
            int j = n;
            while (j > i)
            {
                auto it = vocab.token_to_id.find(word.substr(i, j - i));
                if (it != vocab.token_to_id.end())
                {
                    tokens.push_back(it->second);
                    i = j;
                    break;
                }
                --j;
            }
            if (i == n)
            {
                break;
            }
            if (j == i)
            {
                auto sub = word.substr(i, 1);
                if (vocab.token_to_id.find(sub) != vocab.token_to_id.end())
                {
                    tokens.push_back(vocab.token_to_id.at(sub));
                }
                else
                {
                    fprintf(stderr, "%s: unknown token '%s'\n", __func__, sub.data());
                }
                ++i;
            }
        }
    }

    return tokens;
}

bool gpt_vocab_init(const std::string &fname, gpt_vocab &vocab)
{
    printf("%s: loading vocab from '%s'\n", __func__, fname.c_str());

    vocab.token_to_id = ::json_parse(fname);

    for (const auto &kv : vocab.token_to_id)
    {
        vocab.id_to_token[kv.second] = kv.first;
    }

    printf("%s: vocab size = %d\n", __func__, (int)vocab.token_to_id.size());

    // print the vocabulary
    // for (auto kv : vocab.token_to_id) {
    //    printf("'%s' -> %d\n", kv.first.data(), kv.second);
    //}

    return true;
}

gpt_vocab::id gpt_sample_top_k_top_p(
    const gpt_vocab &vocab,
    const float *logits,
    int top_k,
    double top_p,
    double temp,
    std::mt19937 &rng)
{
    int n_logits = vocab.id_to_token.size();

    std::vector<std::pair<double, gpt_vocab::id>> logits_id;
    logits_id.reserve(n_logits);

    {
        const double scale = 1.0 / temp;
        for (int i = 0; i < n_logits; ++i)
        {
            logits_id.push_back(std::make_pair(logits[i] * scale, i));
        }
    }

    // find the top K tokens
    std::partial_sort(
        logits_id.begin(),
        logits_id.begin() + top_k, logits_id.end(),
        [](const std::pair<double, gpt_vocab::id> &a, const std::pair<double, gpt_vocab::id> &b)
        {
            return a.first > b.first;
        });

    logits_id.resize(top_k);

    double maxl = -INFINITY;
    for (const auto &kv : logits_id)
    {
        maxl = std::max(maxl, kv.first);
    }

    // compute probs for the top K tokens
    std::vector<double> probs;
    probs.reserve(logits_id.size());

    double sum = 0.0;
    for (const auto &kv : logits_id)
    {
        double p = exp(kv.first - maxl);
        probs.push_back(p);
        sum += p;
    }

    // normalize the probs
    for (auto &p : probs)
    {
        p /= sum;
    }

    if (top_p < 1.0f)
    {
        double cumsum = 0.0f;
        for (int i = 0; i < top_k; i++)
        {
            cumsum += probs[i];
            if (cumsum >= top_p)
            {
                top_k = i + 1;
                probs.resize(top_k);
                logits_id.resize(top_k);
                break;
            }
        }

        cumsum = 1.0 / cumsum;
        for (int i = 0; i < (int)probs.size(); i++)
        {
            probs[i] *= cumsum;
        }
    }

    // printf("\n");
    // for (int i = 0; i < (int) probs.size(); i++) {
    //     printf("%d: '%s' %f\n", i, vocab.id_to_token.at(logits_id[i].second).c_str(), probs[i]);
    // }
    // exit(0);

    std::discrete_distribution<> dist(probs.begin(), probs.end());
    int idx = dist(rng);

    return logits_id[idx].second;
}
