#include "OV7670.h"

#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library

#include <Base64.h>
#include <FS.h>
#include "SPIFFS.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "BMP.h"

extern "C" {
#include "crypto/base64.h"
}

//IPAddress myIP       = IPAddress(192, 168, 1, 66);
//IPAddress myGateway = IPAddress(192, 168, 1, 125);

const int XRESS = 160;
const int YRESS = 120;
const int BYTES_PER_PIXEL = 2;
unsigned char frame[XRESS  * YRESS  * BYTES_PER_PIXEL];

const int SIOD = 21; //SDA
const int SIOC = 22; //SCL

const int VSYNC = 13; //34;
const int HREF = 4; //35;

const int XCLK = 27; //32;
const int PCLK = 14; //33;

const int D0 = 36; //27;
const int D1 = 39; //17;
const int D2 = 34; //16;
const int D3 = 35; //15;
const int D4 = 32; //14;
const int D5 = 33; //13;
const int D6 = 25; //12;
const int D7 = 26; //4;

//const int TFT_DC = 2;
//const int TFT_CS = 5;
//DIN <- MOSI 23
//CLK <- SCK 18

#define ssid1        "NomeDaSuaRede"
#define password1    "SenhaDaSuaRede"
//#define ssid2        ""
//#define password2    ""

//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, 0/*no reset*/);
OV7670 *camera;

WiFiMulti wifiMulti;
WiFiServer server(80);
// Use WiFiClient class to create TCP connections
WiFiClient client;

String timeIni,timeFim; //VARIÁVEIS UTILIZADAS PARA CONTAR O TEMPO DE ESPERA PARA ENVIO DE FOTO
int envio = 0;

unsigned char bmpHeader[BMP::headerSize];

void http_request(){
    const char* host = "SeuHostAqui"; //"192.168.0.25";

    //Gravando Imagem na Memória
    File file = SPIFFS.open("/teste.bmp", FILE_WRITE); // Here the file is opened

    if (!file) {
        Serial.println("Error opening the file."); // Good practice to check if the file was correctly opened
        return; // If file not opened, do not proceed
    }

    for (int i = 0; i <BMP :: headerSize; i ++)
        file.write(bmpHeader [i]); // Writes header information to the BMP file

    for (int i = 0; i <camera-> xres * camera-> yres * 2; i ++)
        file.write(camera-> frame [i]); // Writes pixel information to the BMP file

    file.close(); // Closing the file saves its content

    //Lendo Imagem gravada na memória
    file = SPIFFS.open("/teste.bmp", FILE_READ);
    int fileSize = (int)file.size();
    Serial.println("Filesize: " + String(fileSize));
    
    Serial.print("connecting to ");
    Serial.println(host);

    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    } else {
       Serial.println("conectado");
    }

    // We now create a URI for the request
    String url = "/registrarMonitoramento";//"/server/pageServer.php";
    //String PostData="imagem=esptestenovo.png";
    //int contentLength = PostData.length();

    Serial.print("Requesting URL: ");
    Serial.println(url);

    const char* boundry = "345324sdhg3444";
    const char* filenamePhoto = "/teste.bmp"; // Faça upload do arquivo no menu: Tools -> "ESP8266 Sketch Data Upload"
    String start_request = "";
    String end_request = "";

    // Delimitador de conteúdo inicial "MAC"
    start_request = start_request + "--" + boundry + "\r\n";
    start_request = start_request + "content-disposition: form-data; name=\"iptBoxSerial\"" + "\r\n"; //\"mac\"" + "\r\n"; //name = iptBoxSerial
    start_request = start_request + "\r\n";
    start_request = start_request + "1234567890\r\n";

    // Delimitador de conteúdo inicial "photo"
    start_request = start_request + "--" + boundry + "\r\n";
    start_request = start_request + "content-disposition: form-data; name=\"iptFoto\"; filename=\"teste.bmp\"\r\n"; //\"teste.bmp\"\r\n"; //name= iptFoto
    start_request = start_request + "Content-Type: image/bmp\r\n";
    start_request = start_request + "\r\n";

    // Delimitador de conteúdo final
    end_request = end_request + "\r\n";
    end_request = end_request + "--" + boundry + "--" + "\r\n";

    int contentLength = (int)file.size() + start_request.length() + end_request.length();

    String headers = String("POST ") + url + " HTTP/1.1\r\n";
    headers = headers + "Host: SeuHostAqui \r\n"; //192.168.0.25 \r\n";
    headers = headers + "User-Agent: ESP8266\r\n";
    headers = headers + "Accept: */*\r\n";
    headers = headers + "Content-Type: multipart/form-data; boundary=" + boundry + "\r\n";
    headers = headers + "Content-Length: " + contentLength + "\r\n";
    headers = headers + "\r\n";
    headers = headers + "\r\n";

    Serial.println();
    Serial.println("Enviando dados para o Telegram...");        

    // Envia cabeçalho HTTP
    Serial.print(headers);        
    client.print(headers);
    client.flush();

    // Envia delimitador de conteúdo inicial
    Serial.print(start_request);
    client.print(start_request);
    client.flush();

    // Envia conteúdo do arquivo
    //sendFile(&file);
    client.write(file);

    file.close();
    client.flush();

    // Envia delimitador de conteúdo final
    Serial.print(end_request);
    client.print(end_request);
    client.flush();
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");
}

// Envia os dados do arquivo via stream
void sendFile(Stream* stream)
{
    size_t bytesReaded; // bytes lidos
    size_t bytesSent;   // bytes enviados
    do {
        uint8_t buff[1024]; // buffer
        bytesSent = 0;
        bytesReaded = stream->readBytes(buff, sizeof(buff));
        if (bytesReaded) {
            bytesSent = client.write(buff, bytesReaded);
            client.flush();
        }        
    } while ( (bytesSent == bytesReaded) && (bytesSent > 0) );
}

void serve()
{
  WiFiClient client = server.available();
  if (client) 
  {
    //Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        //Serial.write(c);
        if (c == '\n') 
        {
          if (currentLine.length() == 0) 
          {
            /*client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();*/
            /*client.print(
              "<style>body{margin: 0}\nimg{height: 100%; width: auto}</style>"
              "<img id='a' src='/camera' onload='this.style.display=\"initial\"; var b = document.getElementById(\"b\"); b.style.display=\"none\"; b.src=\"camera?\"+Date.now(); '>"
              "<img id='b' style='display: none' src='/camera' onload='this.style.display=\"initial\"; var a = document.getElementById(\"a\"); a.style.display=\"none\"; a.src=\"camera?\"+Date.now(); '>");*/
            /*File file = SPIFFS.open("/automacao.html", FILE_READ);

            client.write(file);
            client.println();
            
            file.close();*/
            break;
          } 
          else 
          {
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        {
          currentLine += c;
        }

        if(currentLine.endsWith("GET /index.html")) {
              Serial.println("Solicitando index");
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              
              File file = SPIFFS.open("/automacao.html", FILE_READ);

              client.write(file);
              client.println();
              
              file.close();
        }

        if(currentLine.endsWith("GET /live")) {
              //client.println("HTTP/1.1 200 OK");
              //client.println("Content-type:text/html");
              //client.println();
              client.print(
              /*"<style>body{margin: 0}\nimg{height: 100%; width: auto}</style>"*/
              "<img id='a' src='/camera' onload='this.style.display=\"initial\"; var b = document.getElementById(\"b\"); b.style.display=\"none\"; b.src=\"camera?\"+Date.now(); '>"
              "<img id='b' style='display: none' src='/camera' onload='this.style.display=\"initial\"; var a = document.getElementById(\"a\"); a.style.display=\"none\"; a.src=\"camera?\"+Date.now(); '>");
        }

        /*if(currentLine.endsWith("GET /jquery.min.js")) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:application/javascript");
          Serial.println("application/javascript");
          File file = SPIFFS.open("/jquery.min.js", FILE_READ);

          client.write(file);
          String filename = file.name();
          Serial.println("Filename: " + filename);
            
          file.close();

          Serial.println(currentLine.c_str());
        }*/

        if(currentLine.endsWith("GET /teste"))
        {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:image/bmp");
            client.println("Transfer-Encoding:identity");
            client.println();

            /*File file = SPIFFS.open("/teste.bmp", FILE_WRITE); // Here the file is opened

            if (!file) {
                Serial.println("Error opening the file."); // Good practice to check if the file was correctly opened
                return; // If file not opened, do not proceed
            }*/

            /*for (int i = 0; i <BMP :: headerSize; i ++)
              file.write(bmpHeader [i]); // Writes header information to the BMP file

            for (int i = 0; i <camera-> xres * camera-> yres * 2; i ++)
              file.write(camera-> frame [i]); // Writes pixel information to the BMP file*/

            //file.close(); // Closing the file saves its content

            File file = SPIFFS.open("/teste.bmp", FILE_READ);

            client.write(file);
            
            file.close();

            //Se desejar acionar a captura de uma imagem através de uma url  
            envio = 1;
        }

        if(currentLine.endsWith("GET /jquery.min.js")){
          Serial.println("Entrou aqui");
        }
        
        if(currentLine.endsWith("GET /camera"))
        {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:image/bmp");
            client.println("Transfer-Encoding:identity");
            client.println();
            
            client.write(bmpHeader, BMP::headerSize);
            client.write(camera->frame, camera->xres * camera->yres * 2);

            //size_t size = image.size;
            size_t size = (camera->xres * camera->yres * 2);
            //const uint8_t* image = image.data;
            const uint8_t *image = camera->frame;
            
            File file = SPIFFS.open("/teste.bmp", FILE_WRITE); // Here the file is opened

            if (!file) {
            Serial.println("Error opening the file."); // Good practice to check if the file was correctly opened
            return; // If file not opened, do not proceed
            }

            for (int i = 0; i <BMP :: headerSize; i ++)
            {
              file.write(bmpHeader [i]); // Writes header information to the BMP file
            }

            for (int i = 0; i <camera-> xres * camera-> yres * 2; i ++)
            {
              file.write(camera-> frame [i]); // Writes pixel information to the BMP file
            }

            file.close(); // Closing the file saves its content            
            
            //String tempstring = reinterpret_cast<const char*>(camera->frame);
            //Serial.println(tempstring);
            //Serial.println(*camera->frame);
            //Serial.println(camera->xres * camera->yres * 2);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }  
}

void setup() 
{
  Serial.begin(115200);
  
  wifiMulti.addAP(ssid1, password1);
  //wifiMulti.addAP(ssid2, password2);
  Serial.println("Connecting Wifi...");
  if(wifiMulti.run() == WL_CONNECTED) {
      //WiFi.config( myIP, myGateway, IPAddress(255, 255, 255, 0));
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
  }
  camera = new OV7670(OV7670::Mode::QQVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);
  
  //tft.initR(INITR_BLACKTAB);
  //tft.fillScreen(0);
  server.begin();
  SPIFFS.begin();
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
  }
  

  //Tempo Inicial de Processamento
  timeIni = String(millis());
}

/*void displayY8(unsigned char * frame, int xres, int yres)
{
  tft.setAddrWindow(0, 0, yres - 1, xres - 1);
  int i = 0;
  for(int x = 0; x < xres; x++)
    for(int y = 0; y < yres; y++)
    {
      i = y * xres + x;
      unsigned char c = frame[i];
      unsigned short r = c >> 3;
      unsigned short g = c >> 2;
      unsigned short b = c >> 3;
      tft.pushColor(r << 11 | g << 5 | b);
    }  
}*/

/*void displayRGB565(unsigned char * frame, int xres, int yres)
{
  tft.setAddrWindow(0, 0, yres - 1, xres - 1);
  int i = 0;
  for(int x = 0; x < xres; x++)
    for(int y = 0; y < yres; y++)
    {
      i = (y * xres + x) << 1;
      tft.pushColor((frame[i] | (frame[i+1] << 8)));
    }  
}*/

void loop()
{
  
  camera->oneFrame();
  serve();

  timeFim = String(millis());
  if(timeFim.toInt() - timeIni.toInt() >= 30000){ //Enviar uma imagem a cada 30 segundos
    envio = 1;

    timeIni = String(millis());
  }
  
  if(envio == 1){
      http_request();

      envio = 0;
  }
  
  //enviaDados();
  //displayRGB565(camera->frame, camera->xres, camera->yres);
}
