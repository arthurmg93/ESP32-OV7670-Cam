# ESP32-OV7670(noFIFO)-Cam
Usando microcontrolador ESP32 e o módulo OV7670 (No FIFO) para Upload de imagem para um host Http


Este projeto começou e só teve algum sucesso graças ao trabalho do BitLuni (https://bitluni.net/esp32-i2s-camera-ov7670).
Graças a ele foi possível fazer a captura de imagem com o módulo câmera OV7670. No tutorial dele é possível encontrar todos os detalhes sobre conexão do ESP32 com a câmera OV7670.


Lembre-se de alterar no código as informações da sua rede WiFi:

#define ssid1        "NomeDaSuaRede"
#define password1    "SenhaDaSuaRede"


Também será necessário alterar o Host para onde as imagens no formato BMP (BitMap) serão enviadas.

const char* host = "SeuHostAqui"
.
.
.
headers = headers + "Host: SeuHostAqui \r\n"


Ao final do código é possível alterar a frequência de Upload de imagens em milisegundos:
timeFim.toInt() - timeIni.toInt() >= 30000 //Enviar uma imagem a cada 30 segundos
