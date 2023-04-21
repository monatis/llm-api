# llm-api
Web API and websocket for Large Language Models in C++

## Buıld

1. clone the repo and cd into it:

```shell
git clone https://github.com/monatis/llm-api.git && cd llm-api
```

2. Install `asio` for the web API.

```shell
apt install libasio-dev
```

**Note**: You can also run `scripts/install-dev.sh` to install asio 
(and websocat additionally, in order to test websocket on the terminal).

3. Build with cmake and make:

```shell
mkdir build && cd build
cmake -DLLM_NATIVE=ON ..
make -j4
```

Find the executible in `./bin/llm-api`.

## Run

1. Download gpt4all-j model if you haven't already:

```shell
wget https://gpt4all.io/models/ggml-gpt4all-j.bin -O ./bin/ggml-gpt4all-j.bin
```

2. Run the executible:

```shell
./bin/llm-api
```

**Note**: You can pass the model path with `-m` argument if it's located elsewhere. See below for more options.

## Options

```
./bin/llm-api -h

usage: ./bin/llm-api [options]                                                                                          
                                                                                                                        
options:
  -h, --help            show this help message and exit
  -v, --verbose            log generation in stdout (default: disabled)
  -s SEED, --seed SEED  RNG seed (default: -1)
  -t N, --threads N     number of threads to use during computation (default: 4)
  --port PORT     port to listen on (default: 8080)
  -p PROMPT, --prompt PROMPT                                                                                            
                        prompt to start generation with (default: random)
  -n N, --n_predict N   number of tokens to predict (default: 200)
  --top_k N             top-k sampling (default: 40)
  --top_p N             top-p sampling (default: 0.9)
  --temp N              temperature (default: 0.9)
  -b N, --batch_size N  batch size for prompt processing (default: 8)
  -m FNAME, --model FNAME                                                                                               
                        model path (default: ggml-gpt4all-j.bin)
```

## Roadmap

- [ ] Improve multi-user experience
- [ ] Add Docker support
- [ ] Integrate a chat UI.
- [ ] Add embedding endpoint.
- [ ] Integrate StableLM model.
- [ ] Provide a chain mechanism.
