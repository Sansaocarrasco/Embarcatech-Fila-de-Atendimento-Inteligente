# Fila de Atendimento Inteligente

Este projeto visa criar uma solução prática para o gerenciamento de filas de atendimento, com o uso de uma Raspberry Pi Pico W, display OLED, buzzer e botões de controle. O sistema organiza e exibe filas de atendimento comum e prioritário, com interações via botões físicos e comunicação USB.

## 🛠 Componentes Utilizados

A tarefa requer os seguintes componentes conectados à Raspberry Pi Pico W:

| Componente               | Conexão à GPIO    |
|--------------------------|------------------|
| Raspberry Pi Pico W      | Controlador central |
| Botão A                  | Entrada para interação do usuário |
| Botão B                  | Controle do buzzer |
| Display OLED SSD1306     | I2C (GPIO 14, GPIO 15) |
| Buzzer                   | Emissão de som |
| Comunicação USB          | Interface de comunicação |

## 📌 Requisitos da Atividade

1. **Desenvolver um sistema de controle de filas de atendimento**:
   - O sistema deve gerenciar duas filas distintas: uma para atendimentos comuns e outra para atendimentos prioritários.
   - Os números das filas serão armazenados e exibidos sequencialmente.

2. **Exibição no Display OLED**:
   - O número atual e o próximo a ser atendido devem ser exibidos no display SSD1306.
   
3. **Interação via Botões**:
   - **Botão A**: Utilizado para confirmar o atendimento de um número da fila.
   - A lógica de atendimento segue a regra: a cada dois atendimentos comuns, o próximo será um prioritário.
   - **Botão B**: Alterna o estado do buzzer, que emite um som durante o atendimento.

4. **Emissão de Som via Buzzer**:
   - Um alerta sonoro será emitido sempre que um novo número for chamado.

5. **Comunicação USB**:
   - Permitir o envio de números para a fila através de comunicação via USB.

## 🎯 Justificativa

A necessidade de soluções eficientes para o gerenciamento de filas é cada vez mais relevante, especialmente em locais de grande fluxo, como hospitais, bancos e estabelecimentos comerciais. Este projeto oferece uma solução acessível e simples, integrando hardware de baixo custo com uma interface de fácil utilização.

## 💡 Originalidade

Embora existam sistemas de gerenciamento de filas, a integração de um display OLED, botões físicos e comunicação USB para gerenciamento de filas utilizando a Raspberry Pi Pico W ainda não é amplamente difundida. O projeto se destaca por sua simplicidade e custo reduzido.

## 🎥 Vídeo Demonstrativo

O vídeo demonstrativo deste projeto pode ser acessado no link a seguir:

https://youtu.be/8jKBWqoP_Y8?si=ImBj8muZwuEUqSMz

*Fonte: autor*

## 📜 Licença

Este projeto está licenciado sob a Licença MIT. Veja o arquivo `LICENSE` para mais detalhes.
