<div align="center">

# Marco 3 · Classificador ELM na DE1-SoC

**Coprocessador ELM em Verilog · VGA · C no HPS · ARM Assembly · MMIO**

[O que é?](#o-que-e) ·
[Hardware](#hardware-coprocessador--controlador-vga) ·
[Como compilar](#como-compilar) ·
[Funções](#funcoes)

</div>

---

## O que é?

<a id="o-que-e"></a>

<details>
<summary><strong>Abrir: visão geral do projeto</strong></summary>

Este repositório contém a versão do Marco 3 do classificador de dígitos para a DE1-SoC. A inferência da rede ELM roda em um coprocessador implementado na FPGA, enquanto o HPS Linux controla o hardware por MMIO usando uma aplicação em C e rotinas em ARM Assembly.

Além do coprocessador, o Marco 3 integra o controlador VGA e um modo de desenho com mouse. A aplicação final permite:

- enviar uma imagem `.raw` de 784 bytes e classificar o dígito;
- desenhar um dígito no VGA usando mouse e classificar o desenho;
- rodar um teste rápido de sanidade com imagens conhecidas;
- rodar o `testbranch`, que avalia imagens PNG organizadas por classe e gera logs CSV.

Organização usada no repositório entregue:

```text
HARDWARE/
  Verilog, projeto Quartus, coprocessador ELM e controlador VGA

MENU_API_C/
  aplicação C, headers, rotinas .S, leitura de PNG, mouse e benchmark
```

<p align="center">
  <img src="https://imagedelivery.net/8gVZthzfxP_cknrY6e0ejQ/b9f299b4-4891-4087-5e8c-9a6da4ac3800/public" alt="Imagem 1 - visão geral do sistema" width="720">
</p>

<p align="center"><em>Imagem 1: visão geral do fluxo HPS, MMIO, coprocessador ELM e VGA.</em></p>

Arquivos de modelo esperados no mesmo diretório do executável na placa:

```text
W_in.raw
b.raw
beta.raw
```

Formato base de imagem para o coprocessador:

```text
28 x 28 pixels = 784 bytes, 1 byte por pixel, escala de cinza
```

</details>

---

## Hardware: Coprocessador + Controlador VGA

<a id="hardware-coprocessador--controlador-vga"></a>

<details>
<summary><strong>Abrir: projeto Quartus, PIOs e mapa MMIO</strong></summary>

O hardware fica na pasta `HARDWARE/`. O HPS acessa os blocos da FPGA pela Lightweight Bridge, usando a base física:

```c
#define NV_LW_BASE 0xFF200000u
#define NV_LW_SPAN 0x00010000u
```

<details>
<summary><strong>PIOs usados pelo Marco 3</strong></summary>

| Bloco | PIO | Offset | Direção no HPS | Uso |
|---|---:|---:|---|---|
| ELM | `data_in` | `0x40` | escrita | palavra de instrução de 32 bits para o coprocessador |
| ELM | `data_out` | `0x50` | leitura | dígito previsto e flags `DONE/BUSY/ERROR` |
| ELM | `ctrl` | `0x60` | escrita | pulsos de `enable`, `clr_operation` e `rst` |
| VGA | `vga_data` | `0x70` | escrita | posição `(x,y)` e cor RGB 3 bits |
| VGA | `vga_ctrl` | `0x80` | escrita | pulso de escrita de pixel |
| VGA | `vga_status` | `0x90` | leitura | `done` do controlador VGA |

Mapa resumido:

```text
0xFF200000
├── +0x40  data_in
├── +0x50  data_out
├── +0x60  ctrl
├── +0x70  vga_data
├── +0x80  vga_ctrl
└── +0x90  vga_status
```

<p align="center">
  <img src="https://imagedelivery.net/8gVZthzfxP_cknrY6e0ejQ/286899f7-23d9-46dd-f20e-a2ecb811fc00/public" alt="Imagem 2 - mapa MMIO do Marco 3" width="720">
</p>

<p align="center"><em>Imagem 2: mapa dos PIOs do coprocessador e do controlador VGA.</em></p>

</details>

<details>
<summary><strong>Controle do coprocessador ELM</strong></summary>

O PIO `ctrl` chega ao top-level separado em três sinais:

```verilog
wire elm_enable = ctrl_w[0];
wire elm_clr    = ctrl_w[1];
wire elm_rst    = ctrl_w[2];
```

Valores escritos pela API:

| Valor em `ctrl` | Sinal | Efeito |
|---:|---|---|
| `1` | `enable` | faz o coprocessador capturar `data_in` |
| `2` | `clr_operation` | limpa `DONE/ERROR` |
| `4` | `rst` | reseta as FSMs/controle do coprocessador |
| `0` | nenhum | solta o pulso |

O reset do coprocessador limpa estado de controle. Ele não recarrega `W_in`, `b`, `beta` e não apaga as RAMs usadas como memória do modelo.

</details>

<details>
<summary><strong><code>data_out</code> do coprocessador</strong></summary>

Bits usados pela API:

| Bits | Campo |
|:---:|---|
| `[3:0]` | dígito previsto |
| `[4]` | `DONE` |
| `[5]` | `BUSY` |
| `[6]` | `ERROR` |

Máscaras em `nv_hw.h`:

```c
#define NV_MASK_DIGIT  0xFu
#define NV_MASK_DONE   (1u << 4)
#define NV_MASK_BUSY   (1u << 5)
#define NV_MASK_ERROR  (1u << 6)
```

</details>

<details>
<summary><strong>Controlador VGA</strong></summary>

O VGA recebe uma escrita de pixel por vez. O dado enviado em `vga_data` contém posição e cor:

```c
return (x & 0x1FFu)
     | ((y & 0xFFu) << 9)
     | ((r3 & 7u) << 17)
     | ((g3 & 7u) << 20)
     | ((b3 & 7u) << 23);
```

Campos:

| Bits | Campo |
|:---:|---|
| `[8:0]` | `x` |
| `[16:9]` | `y` |
| `[19:17]` | vermelho |
| `[22:20]` | verde |
| `[25:23]` | azul |

Protocolo de escrita:

```text
1. escrever vga_data
2. escrever vga_ctrl = 1
3. esperar vga_status[0] = 1
4. escrever vga_ctrl = 0
```

Resolução usada pela aplicação:

```c
#define VGA_WIDTH   320u
#define VGA_HEIGHT  240u
#define IMG_W       28u
#define IMG_H       28u
```

</details>

<details>
<summary><strong>Arquivos de hardware principais</strong></summary>

Na pasta `HARDWARE/`, os blocos principais são:

- `CoProcessor.v`: decodifica a ISA do ELM e controla `BUSY`, `DONE`, `ERROR`.
- `neural_unit.v`: controla a sequência da rede.
- `first_layer.v`: camada oculta.
- `second_layer.v`: camada de saída.
- `argmax_iterativo.v`: seleção do maior score.
- `lsu_controller.v`: acesso às RAMs internas.
- `controller_vga_to_sd.v`: controlador de escrita no VGA.

</details>

</details>

---

## Como compilar

<a id="como-compilar"></a>

<details>
<summary><strong>Abrir: comandos na placa</strong></summary>

Entre na pasta `MENU_API_C/` na DE1-SoC e compile:

```bash
gcc -O2 -Wall -std=c99 -o nv_app \
  nv_main.c nv_hw.c nv_session.c nv_testbranch.c nv_png_img.c mouse_draw.c \
  nv_reset.S nv_store_pixel.S nv_inference_start.S \
  nv_store_weights.S nv_store_bias.S nv_store_beta.S \
  -lm -lrt
```

Em uma linha:

```bash
gcc -O2 -Wall -std=c99 -o nv_app nv_main.c nv_hw.c nv_session.c nv_testbranch.c nv_png_img.c mouse_draw.c nv_reset.S nv_store_pixel.S nv_inference_start.S nv_store_weights.S nv_store_bias.S nv_store_beta.S -lm -lrt
```

Executar:

```bash
sudo ./nv_app
```

O programa usa `/dev/mem` e, no modo de desenho, também acessa `/dev/input/event*`, então precisa rodar com permissão de root.

<details>
<summary><strong>Compilar o monitor de <code>data_out</code></strong></summary>

`nv_peek` serve para observar o `DATA_OUT` do coprocessador em outro terminal SSH:

```bash
gcc -O2 -Wall -std=c99 -o nv_peek nv_peek.c -lrt
sudo ./nv_peek -w 5
```

</details>

<details>
<summary><strong>Compilar versão 100% C, sem Assembly</strong></summary>

Existe uma versão alternativa do protocolo MMIO sem `.S`, usando `nv_main_pure.c` e `nv_mmio.c`:

```bash
gcc -O2 -Wall -std=c99 -o nv_app_pure \
  nv_main_pure.c nv_hw.c nv_mmio.c nv_session.c \
  -lm -lrt
```

Essa versão é útil para comparar o protocolo C puro com as rotinas ARM Assembly.

</details>

</details>

---

## Funções

<a id="funcoes"></a>

<details>
<summary><strong>Abrir: arquivos C, headers e rotinas Assembly</strong></summary>

<details>
<summary><strong>Menu da aplicação final</strong></summary>

O programa carrega o modelo na inicialização:

```text
W_in.raw
b.raw
beta.raw
```

Menu principal:

```text
1 - Enviar imagem .raw + inferencia
2 - Teste 4x (imagem_3, imagem_3, imagem_1, imagem_1)
3 - Desenhar digito (mouse + VGA)
4 - Testbranch (testes/0..9, 100 .png/pasta -> logs.csv)
```

Variáveis úteis:

```bash
export NV_VERBOSE=1
export MOUSE_DEV=/dev/input/eventN
```

</details>

<details>
<summary><strong><code>nv_mmap_open</code>, <code>nv_mmap_close</code> e <code>nv_read_state</code></strong></summary>

Arquivos: `nv_hw.c`, `nv_hw.h`

`nv_mmap_open()` abre `/dev/mem`, mapeia a Lightweight Bridge e preenche:

```c
ports->data_in  = mapped + 0x40;
ports->ctrl     = mapped + 0x60;
ports->data_out = mapped + 0x50;
```

`nv_read_state()` lê `data_out` e separa:

```text
digit, done, busy, error
```

`nv_print_state()` imprime esse estado para depuração.

</details>

<details>
<summary><strong><code>nv_reset</code>, <code>nv_hw_reset</code> e <code>nv_hw_clear</code></strong></summary>

Arquivos: `nv_reset.S`, `nv_hw.c`

`nv_reset()` pulsa o reset real do coprocessador:

```text
ctrl = 4
ctrl = 0
```

`nv_hw_reset()` chama `nv_reset()` e aplica barreira de sincronização.

`nv_hw_clear()` espera `BUSY=0`, pulsa:

```text
ctrl = 2
ctrl = 0
```

e aguarda `DONE=0`.

</details>

<details>
<summary><strong><code>nv_store_img_pixel</code>, <code>nv_send_image_buf</code> e <code>nv_send_image_raw</code></strong></summary>

Arquivos: `nv_store_pixel.S`, `nv_session.c`

`nv_store_img_pixel()` envia um pixel para a memória de imagem do coprocessador:

```text
[20:13] pixel 8 bits
[12:3]  índice 0..783
[2:0]   opcode STORE_IMG = 000
```

`nv_send_image_buf()` limpa o estado com `nv_hw_clear()`, envia os 784 pixels e limpa novamente.

`nv_send_image_raw()` lê um `.raw` de 784 bytes e chama `nv_send_image_buf()`.

</details>

<details>
<summary><strong>Carga do modelo: <code>nv_model_load</code></strong></summary>

Arquivo: `nv_session.c`

`nv_model_load()` carrega:

| Arquivo | Entradas | Tamanho |
|---|---:|---:|
| `W_in.raw` | 100352 valores `uint16_t` | 200704 bytes |
| `b.raw` | 128 valores `uint16_t` | 256 bytes |
| `beta.raw` | 1280 valores `uint16_t` | 2560 bytes |

Rotinas usadas:

- `nv_weights_send_addr()`
- `nv_weights_send_value()`
- `nv_store_bias()`
- `nv_store_beta()`

Depois da carga, a API chama reset e clear para deixar o coprocessador em estado conhecido.

</details>

<details>
<summary><strong><code>nv_run_inference</code> e <code>nv_inference_start</code></strong></summary>

Arquivos: `nv_session.c`, `nv_inference_start.S`

`nv_run_inference()` organiza o início da inferência:

```text
ler estado inicial
reset do hardware
clear
chamar nv_inference_start
medir latência
ler estado final
```

`nv_inference_start.S` envia `START = 5`, pulsa `enable`, espera `BUSY=1` para confirmar que o coprocessador aceitou a operação, e depois espera `DONE=1`.

<p align="center">
  <img src="https://imagedelivery.net/8gVZthzfxP_cknrY6e0ejQ/fe2046b1-2f68-4fb5-e46c-e143e807fd00/public" alt="Imagem 3 - fluxo de inferência" width="720">
</p>

<p align="center"><em>Imagem 3: sequência de envio da imagem, reset, START, BUSY, DONE e leitura do dígito.</em></p>

</details>

<details>
<summary><strong>PNG: <code>nv_png_img.c</code> e <code>stb_image.h</code></strong></summary>

`nv_load_image_png()` usa `stb_image.h` para decodificar PNG em escala de cinza.

Se a imagem não for 28x28, ela é redimensionada para 28x28 usando interpolação bilinear. O resultado final é sempre:

```text
uint8_t buf[784]
```

`nv_send_image_png()` decodifica a imagem e envia o buffer ao coprocessador pelo mesmo caminho de `nv_send_image_buf()`.

</details>

<details>
<summary><strong>VGA: <code>vga_driver.h</code></strong></summary>

Funções principais:

- `vga_write_pixel()`: escreve um pixel no controlador VGA.
- `vga_clear_screen()`: limpa a tela.
- `vga_draw_image()`: desenha uma imagem 28x28 centralizada.
- `vga_show_image()`: limpa a tela e desenha a imagem.

A escala padrão é:

```c
#define VGA_DEFAULT_SCALE 4u
```

Com isso, a imagem 28x28 é exibida como 112x112 pixels.

<p align="center">
  <img src="https://imagedelivery.net/8gVZthzfxP_cknrY6e0ejQ/5fb629bf-3d6e-45d8-961c-82338792e000/public" alt="Imagem 4 - modo VGA e desenho com mouse" width="720">
</p>

<p align="center"><em>Imagem 4: tela VGA exibindo a imagem/desenho usado na inferência.</em></p>

</details>

<details>
<summary><strong>Desenho com mouse: <code>mouse_draw_canvas</code></strong></summary>

Arquivo: `mouse_draw.c`

Cria um canvas 28x28 no VGA e captura eventos de mouse por `evdev`.

Controles:

```text
botão esquerdo + movimento: desenha
botão direito ou Enter: finaliza
```

Ao final, a função preenche `uint8_t pixels[784]` e a aplicação envia esse buffer ao coprocessador.

</details>

<details>
<summary><strong>Testbranch: <code>nv_testbranch.c</code></strong></summary>

O benchmark espera a estrutura:

```text
testes/
├── 0/
├── 1/
├── ...
└── 9/
```

Cada pasta deve conter imagens `.png` do respectivo dígito. Por padrão, o programa usa até 100 imagens por pasta.

Saídas geradas:

| Arquivo | Conteúdo |
|---|---|
| `logs.csv` | resumo por dígito e total |
| `logs_detalhe.csv` | uma linha por imagem testada |
| `logs_erros.csv` | apenas classificações incorretas |

Métricas calculadas:

```text
acurácia
latência média
desvio padrão
throughput
```

<p align="center">
  <img src="https://imagedelivery.net/8gVZthzfxP_cknrY6e0ejQ/f5fc2b95-4651-40dd-07d0-2fb5a3b27c00/public" alt="Imagem 5 - resultado do testbranch" width="720">
</p>

<p align="center"><em>Imagem 5: exemplo de resultado do benchmark com acurácia, latência e throughput.</em></p>

</details>

</details>

---

## Autores

Henrique Zeu Sa de Moura  
Ricardo Vilas-Bôas Gomes
