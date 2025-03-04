#include <stdio.h>
#include <stdlib.h>
#include <bios.h>
#include <string.h>

#include "network.h"

#define VERSION "0.1 by Yeo Kheng Meng (2023)"

// User configuration
#define CONFIG_FILENAME "doschgpt.ini"
#define API_KEY_LENGTH_MAX 200
#define MODEL_LENGTH_MAX 50
#define PROXY_HOST_MAX 100

char config_apikey[API_KEY_LENGTH_MAX];
char config_model[MODEL_LENGTH_MAX];
float config_req_temperature;
char config_proxy_hostname[PROXY_HOST_MAX];
int config_proxy_port;
uint16_t config_outgoing_start_port;
uint16_t config_outgoing_end_port;
uint16_t config_socketConnectTimeout;
uint16_t config_socketResponseTimeout;

// Command Line Configuration
bool debug_showRequestInfo = false;
bool debug_showRawReply = false;

// Message Request
#define SIZE_MESSAGE_TO_SEND 10240
char * messageToSend;

volatile bool inProgress = true;


// Called when ending the app
void endFunction(){
  free(messageToSend);
  network_stop();

  printf("Ended DOS ChatGPT client\n");
}

// When network received a Break
void networkBreakHandler( ) {
  printf("End\n");
  inProgress = false;
  endFunction();
  exit(1);
}

// Open config file and store the configuration in memory
bool openAndProcessConfigFile(char * filename){
  FILE * configFile;
  #define BUFF_LENGTH 10
  char buff[BUFF_LENGTH];

  configFile = fopen(filename, "r");
  if(configFile == NULL){
    return false;
  }

  fgets(config_apikey, API_KEY_LENGTH_MAX, configFile);

  //Remove trailing newline
  config_apikey[strcspn(config_apikey, "\r\n")] = '\0';

  fgets(config_model, MODEL_LENGTH_MAX, configFile);

  //Remove trailing newline
  config_model[strcspn(config_model, "\r\n")] = '\0';

  fgets(buff, BUFF_LENGTH, configFile);
  config_req_temperature = atof(buff);
  
  fgets(config_proxy_hostname, PROXY_HOST_MAX, configFile);
  //Remove trailing newline
  config_proxy_hostname[strcspn(config_proxy_hostname, "\r\n")] = '\0';

  fgets(buff, BUFF_LENGTH, configFile);
  config_proxy_port = atoi(buff);

  fgets(buff, BUFF_LENGTH, configFile);
  config_outgoing_start_port = strtoul(buff, NULL, 10);

  fgets(buff, BUFF_LENGTH, configFile);
  config_outgoing_end_port = strtoul(buff, NULL, 10);

  fgets(buff, BUFF_LENGTH, configFile);
  config_socketConnectTimeout = strtoul(buff, NULL, 10);

  fgets(buff, BUFF_LENGTH, configFile);
  config_socketResponseTimeout = strtoul(buff, NULL, 10);

  fclose(configFile);
  
  return true;

}

int main(int argc, char * argv[]){
  printf("Started DOS ChatGPT client %s\n\n", VERSION);

  // Process command line arguments -dri and -drr
  for(int i = 0; i < argc; i++){
    char * arg = argv[i];
    if(strstr(arg, "-dri")){
      debug_showRequestInfo = true;
    } else if(strstr(arg, "-drr")){
      debug_showRawReply = true;
    }
  }

  if(openAndProcessConfigFile(CONFIG_FILENAME)){

    printf("Client config details read from file:\n");
    printf("API key contains %d characters\n", strlen(config_apikey));
    printf("Model: %s\n", config_model);
    printf("Request temperature: %0.2f\n", config_req_temperature);
    printf("Proxy hostname: %s\n", config_proxy_hostname);
    printf("Proxy port: %d\n", config_proxy_port);
    printf("Outgoing start port: %u\n", config_outgoing_start_port);
    printf("Outgoing end port: %u\n", config_outgoing_end_port);
    printf("Socket connect timeout: %u ms\n", config_socketConnectTimeout);
    printf("Socket response timeout: %u ms\n", config_socketResponseTimeout);

    printf("\nDebug request info -dri: %d\n", debug_showRequestInfo);
    printf("Debug raw reply -drr: %d\n", debug_showRawReply);

  } else {
    printf("\nCannot open %s config file containing:\nAPI key\nModel\nRequest Temperature\nProxy hostname\nProxy port\nOutgoing start port\nOutgoing end port\nSocket connect timeout (ms)\nSocket response timeout (ms)\n", CONFIG_FILENAME);
    return -2;
  }

  bool status = network_init(config_outgoing_start_port, config_outgoing_end_port, networkBreakHandler, config_socketConnectTimeout, config_socketResponseTimeout);

  if (status) {
    //printf("Network ok\n");
  } else {
    printf("Cannot init network\n");
    return -1;
  }

  messageToSend = (char *) calloc (SIZE_MESSAGE_TO_SEND, sizeof(char));
  if(messageToSend == NULL){
    printf("Cannot allocate memory for messageToSend\n");
    network_stop();
    return -1;
  }

  printf("\nStart talking to ChatGPT. Press ESC to quit.\n");
  printf("Me:\n");

  int currentMessagePos = 0;

  while(inProgress){

    // Detect if kep is pressed
    if ( _bios_keybrd(_KEYBRD_READY) ) {
      char character = _bios_keybrd(_KEYBRD_READ);

      // Detect ESC key for quit
      if(character == 27){
        inProgress = false;
        break;
      }

      // Detect that user has pressed enter
      if(character == '\r'){

        //Don't send empty request
        if(currentMessagePos == 0){
          continue;
        }

        printf("\n");
        COMPLETION_OUTPUT output;
        memset((void*) &output, 0, sizeof(COMPLETION_OUTPUT));
        network_get_completion(config_proxy_hostname, config_proxy_port, config_apikey, config_model, messageToSend, config_req_temperature, &output);

        if(output.error == COMPLETION_OUTPUT_ERROR_OK){
          printf("\nChatGPT:\n");

          //To scan for the newline \n character to convert to actual newline command for printf
          for(int i = 0; i < output.contentLength; i++){
            char currentChar = output.content[i];
            char nextChar = output.content[i + 1];
            
            //ChatGPT has given us a newline, let's print it then advance 2 steps
            if(currentChar == '\\' && nextChar == 'n'){
              printf("\n");
              i++;
            } else {
              printf("%c", currentChar);
            }

          }
          

          printf("\n");

          if(debug_showRequestInfo){
            printf("Outgoing port %d, %d prompt tokens, %d completion tokens\n", output.outPort, output.prompt_tokens, output.completion_tokens);
          }
          
        } else if(output.error == COMPLETION_OUTPUT_ERROR_CHATGPT){
          printf("\nChatGPT Error:\n%.*s\n", output.contentLength, output.content);
        } else {
          printf("\nApp Error:\n%.*s\n", output.contentLength, output.content);
        }

        if(debug_showRawReply){
          printf("%s\n", output.rawData);
        }
        

        printf("\nMe:\n");

        memset(messageToSend, 0, SIZE_MESSAGE_TO_SEND);
        currentMessagePos = 0;
      } else if((character >= ' ') && (character <= '~')){

        if(currentMessagePos >= SIZE_MESSAGE_TO_SEND){
          printf("Reach buffer max\n");
          continue;
        }

        messageToSend[currentMessagePos] = character;
        currentMessagePos++;
        printf("%c", character);
        fflush(stdout);

      //Backspace character
      } else if(character == 8){

        if(currentMessagePos > 0){
          // Remove previous character
          printf("%s", "\b \b");
          currentMessagePos--;
          messageToSend[currentMessagePos] = '\0';

          fflush(stdout);
        }


      }

    }

    // Call this frequently in the "background" to keep network moving
    network_drivePackets();
  }

  

  endFunction();
  return 0;
}