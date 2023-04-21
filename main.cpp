#include "crow.h"
#include "ggml/ggml.h"
#include "gptj.h"
#include "llmodel.h"
#include "utils.h"
#include <functional>
#include <mutex>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

int main(int argc, char **argv)
{
    gpt_params params;
    if (gpt_params_parse(argc, argv, params) == false)
        return 1;

    GPTJ model;
    model.loadModel(params.model, params.n_threads, params.seed);
    if (!model.isModelLoaded())
        return 1;

    crow::SimpleApp app;
    std::unordered_set<crow::websocket::connection *> ws_connections;
    std::mutex mtx;

    CROW_ROUTE(app, "/")
    ([]()
     {
        crow::json::wvalue info({{"name", "Simple gpt4all-j API"}});
        info["version"] = "0.1.0";
        return info; });

    CROW_ROUTE(app, "/generate")
        .methods("POST"_method)([&](const crow::request &req)
                                {
    crow::json::rvalue user_input;

    try {
    user_input = crow::json::load(req.body);
    if (!user_input.has("prompt")) throw std::invalid_argument("no prompt");
    } catch(...) {
        CROW_LOG_ERROR << "Invalid request: " << req.body;
return crow::response(crow::status::BAD_REQUEST); 
    }
        
        generation_params user_params = get_generation_params(params.gen_params, user_input);
        std::string generatedText;

        std::function<bool(const std::string &)> callback = [&](const std::string & text) -> bool {
            if (!text.empty()) generatedText.append(text);
            return true;
        };

        // TODO: enable saving prompt context for chat experience
        LLModel::PromptContext hist;

        model.prompt(user_params.prompt, callback, hist, user_params.n_predict, user_params.top_k, user_params.top_p, user_params.temperature, user_params.n_batch, params.verbose);
        
        crow::json::wvalue resp({{"generatedText", generatedText}});
        return crow::response(resp); });

    CROW_WEBSOCKET_ROUTE(app, "/chat")
        .onopen([&](crow::websocket::connection &conn)
                {
                    CROW_LOG_INFO << "New websocket connection from " << conn.get_remote_ip();

                    conn.userdata(static_cast<void *>(new LLModel::PromptContext));
                    std::lock_guard<std::mutex> _(mtx);
                    ws_connections.insert(&conn);
                    // TODO: also handle context initialization etc. here
                })
        .onclose([&](crow::websocket::connection &conn, const std::string &reason)
                 {
                     CROW_LOG_INFO << "Connection closed: " << reason;
                     std::lock_guard<std::mutex> _(mtx);
                     // TODO: handle persisting context etc. here before removing connection
                     ws_connections.erase(&conn); })
        .onmessage([&](crow::websocket::connection &conn, const std::string &data, bool is_binary)
                   {
                       // TODO: handle binary data here.
crow::json::rvalue user_input;
 
                       try {
                        user_input = crow::json::load(data);
                        if (!user_input.has("prompt")) throw std::invalid_argument("no prompt");
                        } catch(...) {
                        CROW_LOG_ERROR << "Invalid request: " << data;
                        conn.send_text("{\"error\": \"invalid input\"}");
                        conn.close();
                        return;
                       }

                    CROW_LOG_INFO << "New message from " << conn.get_remote_ip() << ": " << data;
                       generation_params user_params = get_generation_params(params.gen_params, user_input);

                       std::function<bool(const std::string &)> callback = [&](const std::string &text) -> bool
                       {
                        // TODO enable early stopping here
                           
                        if (!text.empty()) {
                           crow::json::wvalue gen({{"token", text}});
                           conn.send_text(gen.dump());
                        }
                           
                           return true;
                       };

        // TODO: enable saving prompt context for chat experience
        auto ctx_ptr = static_cast<LLModel::PromptContext*>(conn.userdata());
        if (!ctx_ptr)  {
            CROW_LOG_ERROR << "Lost user data and context: " << conn.get_remote_ip() << " Closing connection....";
            conn.close();
            return;
        }

        model.prompt(user_params.prompt, callback, *ctx_ptr, user_params.n_predict, user_params.top_k, user_params.top_p, user_params.temperature, user_params.n_batch, params.verbose); });

    app.port(params.port).multithreaded().run();

    return 0;
}
