# doschgpt

A proof-of-concept ChatGPT client for DOS. 

<img src="images\doschgpt-front-combined.jpg" width="1000">

Photos of the client running on [my 1984 IBM 5155 Portable PC](https://github.com/yeokm1/retro-configs/tree/master/desktops/ibm-5155) with a 4.77Mhz Intel 8088 CPU with MS-DOS 6.22.

As there are no native HTTPS for DOS, a [HTTP-to-HTTPS proxy](https://github.com/yeokm1/http-to-https-proxy) like this I've written running on a modern machine is needed.

This program is heavily based on sample code in the DOS networking [MTCP library](http://brutmanlabs.org/mTCP/). The program also requires a [DOS Packet Driver](https://en.wikipedia.org/wiki/PC/TCP_Packet_Driver) to be loaded and MTCP to be set for the machine/VM.

**This program was written in a short time as a toy project. It has not been vigorously tested thus is NOT meant for production use.**

## Using the application

Application binary can be found in the `release` directory but do the following first.

1. OpenAI requires an API key to use its APIs. Follow the [intructions on their website](https://platform.openai.com/account/api-keys) to obtain this key before proceeding.

2. Start up [http-to-https-proxy](https://github.com/yeokm1/http-to-https-proxy)

3. The application requires a config file named `doschgpt.ini`. Modify the configuration file to suit your needs in this order. A sample file can be found with the binary.

* API key: Place your key without qoutes
* Model: Language model to use, can use `gpt-3.5-turbo`
* Request Temperature: How random the completion will be. More [details](https://platform.openai.com/docs/guides/chat/instructing-chat-models)
* Proxy hostname: Hostname IP of the proxy
* Proxy port: Proxy Port
* Outgoing start port: Start of a range of randomly selected outgoing port
* Outgoing end port: End of a range of randomly selected outgoing port
* Socket connect timeout (ms): How long to wait when attempting to connect to proxy
* Socket response timeout (ms): How long to wait for OpenAI's servers to reply

4. Ensure that your DOS environment has loaded the following

* Packet Driver
* MTCP Config Environment variable
* MTCP Config file configured by DHCP

5. Just launch `doschgpt.exe` in your machine and fire away. Press the ESC key to leave. Your may use the following arguments for debug use

* `-dri`: Print the outgoing port, number of prompt and completion tokens used after each request
* `-drr`: Display the raw server return headers and json reply

<img src="images\doschgpt-5155-front-start.jpg" width="500">

Parsed options will be displayed.

## Compilation

To compile this application, you have to use Open Watcom which you can download from [here](https://www.openwatcom.org/). Open Watcom 1.9 was used. Since Open Watcom 1.9 is a 32-bit application, it can run on modern Windows 11 64-bit OS.

During installation, Open Watcom may prompt to install the environment variables. I chose to not use that to avoid having that variables being permanent. Instead I use a batch file to set the variables whenever I need to compile.

The program is compiled via a Makefile that is referenced from MTCP.

```bash
# Open cmd.exe
cd doschgpt-code
owsetenv.bat

# To compile
wmake

# To clean
wmake clean
```

This application compiles against the [MTCP library](http://brutmanlabs.org/mTCP/). I have unzipped the latest version [mTCP-src_2022-07-01.zip](http://www.brutman.com/mTCP/download/mTCP-src_2022-07-01.zip) at the time of development to the `mtcpsrc` directory. When Brutman updates this library again in future, simply replace the contents of the `mtcpsrc` directory with the new library.

## Development

I use Visual Studio Code text editor to code for ease of use. For ease of testing, I used a virtual machine to run the application as 16-bit DOS applications and the MTCP network stack cannot run on modern Windows.

<img src="images\doschgpt-vbox.png" width="600">

More details of my setup can be found [here](https://github.com/yeokm1/retro-configs/tree/master/vms/vbox-dos622).

To easily transfer the binary, I used Python to host my build directory as a temporary webserver. Then use the MTCP tool `htget` to fetch the binary.

```bash
# On modern machine with binary
python3 -m http.server 8000

# Run on DOS machine/VM
htget -o doschgpt.exe http://X.X.X.X:8000/doschgpt.exe
```

### Mock proxy

[OpenAI implements rate limits on their API](https://platform.openai.com/docs/guides/rate-limits/overview) hence we should minimise calling their API repeatedly.

To avoid calling the OpenAI's servers during testing, we can mock the server's using this `mockprox.go`Go program that will replay the contents of `reply.txt` whenever the API call is received. 

<img src="images\doschgpt-mock-proxy.png" width="600">

```bash
cp correct.txt reply.txt
go build mockprox.go
mockprox.exe
```

## ChatGPT APIs

Only one ChatGPT API is used which is the chat completion.

```bash
# Test API directly
curl https://api.openai.com/v1/chat/completions -H "Content-Type: application/json" -H "Authorization: Bearer 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" -d '{ "model": "gpt-3.5-turbo", "messages": [{"role": "user", "content": "What is MS-DOS?"}], "temperature": 0.7 }'

# Call through the https proxy for testing
curl --proxy "http://192.168.1.144:8080" https://api.openai.com/v1/chat/completions -H "Content-Type: application/json" -H "Authorization: Bearer sk-EhmTsEsKyH4qHZL2mr3hT3BlbkFJd6AcfdBrujJsBBGPRcQ" -d '{ "model": "gpt-3.5-turbo", "messages": [{"role": "user", "content": "What is MS-DOS?"}], "temperature": 0.7 }'
```